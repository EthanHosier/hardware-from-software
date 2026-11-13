#!/usr/bin/env python3
"""Create/show/remove only the owned, isolated course veth/network namespace.

No bridges, NAT, default routes or physical-interface changes. setup explicitly
turns TX checksum offload OFF on the two new veth devices, so userspace packet
responders receive complete UDP checksums rather than CHECKSUM_PARTIAL artifacts.
"""
import argparse
import fcntl
import ipaddress
import json
import os
import subprocess
from pathlib import Path

NAMESPACE = 'hfs-lab'
HOST = 'hfs-host'
PEER = 'hfs-peer'
NETWORK = ipaddress.ip_network('198.18.0.0/30')
HOST_ADDRESS = '198.18.0.1/30'
PEER_ADDRESS = '198.18.0.2/30'
ALIAS = 'hardware-from-software networking lab v1'
STATE = Path('/run/hfs-network-lab.json')
NAMESPACE_PATH = Path('/run/netns') / NAMESPACE


def run(arguments, capture=False):
    return subprocess.run(arguments, check=True, text=True,
                          stdout=subprocess.PIPE if capture else None).stdout


def ip_json(*arguments, namespace=False):
    prefix = ['ip', 'netns', 'exec', NAMESPACE] if namespace else []
    return json.loads(run(prefix + ['ip', '-j', *arguments], capture=True))


def names():
    return {line.split()[0] for line in run(['ip', 'netns', 'list'], capture=True).splitlines() if line.strip()}


def links(namespace=False):
    return {link['ifname']: link for link in ip_json('link', 'show', namespace=namespace)}


def ensure_owned(state):
    if state.get('schema') != 1 or state.get('namespace') != NAMESPACE:
        raise RuntimeError('Unexpected ownership record; refusing any deletion.')
    identity = NAMESPACE_PATH.stat()
    if [identity.st_dev, identity.st_ino] != state['namespace_identity']:
        raise RuntimeError('Namespace identity changed; refusing any deletion.')
    host = links().get(HOST, {})
    peer = links(namespace=True).get(PEER, {})
    for link, expected_index in ((host, state['host_ifindex']), (peer, state['peer_ifindex'])):
        if link.get('ifalias') != ALIAS or link.get('ifindex') != expected_index:
            raise RuntimeError('Lab interface ownership changed; refusing any deletion.')


def show(state):
    ensure_owned(state)
    print(json.dumps({'event': 'lab', 'namespace': NAMESPACE,
                      'host': ip_json('address', 'show', 'dev', HOST),
                      'peer': ip_json('address', 'show', 'dev', PEER, namespace=True),
                      'host_checksum_features': run(['ethtool', '-k', HOST], capture=True),
                      'peer_checksum_features': run(['ip', 'netns', 'exec', NAMESPACE, 'ethtool', '-k', PEER],
                                                    capture=True),
                      'note': 'Software link only; not evidence of physical Ethernet DMA/PHY behavior.'}))


def setup():
    if STATE.exists() or NAMESPACE in names() or HOST in links() or PEER in links():
        raise RuntimeError(
            'A lab ownership record, namespace or interface already exists; inspect it rather than replacing it.')
    for interface in ip_json('address', 'show'):
        for address in interface.get('addr_info', []):
            if address.get('family') == 'inet':
                existing = ipaddress.ip_interface(f"{address['local']}/{address['prefixlen']}")
                if existing.network.overlaps(NETWORK):
                    raise RuntimeError(f'Existing address {existing} overlaps the lab subnet; no changes made.')
    for route in ip_json('route', 'show', 'table', 'all'):
        destination = route.get('dst', 'default')
        if destination == 'default':
            continue
        try:
            existing = ipaddress.ip_network(destination, strict=False)
        except ValueError:
            continue
        if existing.version == 4 and existing.prefixlen != 0 and existing.overlaps(NETWORK):
            raise RuntimeError(f'Existing route {destination} overlaps the lab subnet; no changes made.')
    created_namespace = False
    created_link = False
    created_state = False
    try:
        run(['ip', 'netns', 'add', NAMESPACE])
        created_namespace = True
        run(['ip', 'link', 'add', HOST, 'numrxqueues', '1', 'numtxqueues', '1',
             'type', 'veth', 'peer', 'name', PEER, 'numrxqueues', '1', 'numtxqueues', '1'])
        created_link = True
        run(['ip', 'link', 'set', 'dev', HOST, 'alias', ALIAS])
        run(['ip', 'link', 'set', 'dev', PEER, 'alias', ALIAS])
        run(['ip', 'link', 'set', PEER, 'netns', NAMESPACE])
        run(['ip', 'address', 'add', HOST_ADDRESS, 'dev', HOST])
        run(['ip', 'link', 'set', HOST, 'up'])
        run(['ip', 'netns', 'exec', NAMESPACE, 'ip', 'address', 'add', PEER_ADDRESS, 'dev', PEER])
        run(['ip', 'netns', 'exec', NAMESPACE, 'ip', 'link', 'set', PEER, 'up'])
        run(['ip', 'netns', 'exec', NAMESPACE, 'ip', 'link', 'set', 'lo', 'up'])
        print(json.dumps({'event': 'offload_change', 'interfaces': [HOST, PEER],
                          'tx_checksumming': 'off', 'scope': 'new owned veth interfaces only'}), flush=True)
        run(['ethtool', '-K', HOST, 'tx', 'off'])
        run(['ip', 'netns', 'exec', NAMESPACE, 'ethtool', '-K', PEER, 'tx', 'off'])
        for command in (['ethtool', '-k', HOST], ['ip', 'netns', 'exec', NAMESPACE, 'ethtool', '-k', PEER]):
            if 'tx-checksumming: off' not in run(command, capture=True):
                raise RuntimeError('Could not verify disabled TX checksum offload on both owned peers.')
        identity = NAMESPACE_PATH.stat()
        state = {'schema': 1, 'namespace': NAMESPACE,
                 'namespace_identity': [identity.st_dev, identity.st_ino],
                 'host_ifindex': links()[HOST]['ifindex'],
                 'peer_ifindex': links(namespace=True)[PEER]['ifindex']}
        with STATE.open('x') as stream:
            created_state = True
            json.dump(state, stream)
        show(state)
    except BaseException:
        # These flags identify resources created by this invocation, not merely
        # objects with convenient names that existed before it ran.
        if created_link:
            subprocess.run(['ip', 'link', 'delete', HOST], check=False)
        if created_namespace:
            subprocess.run(['ip', 'netns', 'delete', NAMESPACE], check=False)
        if created_state:
            STATE.unlink()
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=['setup', 'show', 'cleanup'])
    args = parser.parse_args()
    if os.geteuid() != 0:
        parser.error('Run with sudo on Linux; namespace operations require root.')
    with Path('/run/hfs-network-lab.lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        if args.action == 'setup':
            setup()
            return
        if not STATE.exists():
            raise RuntimeError('No ownership record; refusing to inspect/delete unrelated similarly named objects.')
        state = json.loads(STATE.read_text())
        ensure_owned(state)
        if args.action == 'show':
            show(state)
            return
        if run(['ip', 'netns', 'pids', NAMESPACE], capture=True).strip():
            raise RuntimeError('Namespace has running processes; stop your lab programs before cleanup.')
        run(['ip', 'link', 'delete', HOST])
        run(['ip', 'netns', 'delete', NAMESPACE])
        STATE.unlink()
        print(json.dumps({'event': 'cleaned', 'namespace': NAMESPACE}))


if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, OSError, subprocess.CalledProcessError) as error:
        raise SystemExit(str(error)) from error
