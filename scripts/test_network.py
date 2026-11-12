#!/usr/bin/env python3
"""Run finite privileged networking tests on the helper-owned namespace/veth lab.

Run after build_network.py, with sudo on Linux. Missing kernel features or
privileges fail the run; they are never reported as successful skipped tests.
No physical interface, default route, NIC driver or real Pi is modified.
"""
import json
import os
import platform
import select
import socket
import struct
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / '.build/network'
NAMESPACE = 'hfs-lab'
INTERFACE = 'hfs-peer'
PEER = '198.18.0.2'
HOST = '198.18.0.1'
PORT = 40420
CHILDREN = []
RESULTS = []


def run(arguments, timeout=30, check=True):
    process = subprocess.run(list(map(str, arguments)), cwd=ROOT, text=True,
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)
    if check and process.returncode != 0:
        raise RuntimeError('Command failed: ' + repr(list(map(str, arguments))) + '\n' + process.stdout)
    return process


def inside(arguments):
    return ['ip', 'netns', 'exec', NAMESPACE, *map(str, arguments)]


def launch(name, *arguments):
    process = subprocess.Popen(inside([BUILD / name, *arguments]), cwd=ROOT,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               text=True, bufsize=1)
    CHILDREN.append(process)
    return process


def ready(process, timeout=8):
    # Programs print one flushed READY line only after their resources are live.
    deadline = time.monotonic() + timeout
    output = b''
    while time.monotonic() < deadline:
        if select.select([process.stdout], [], [], max(0, deadline - time.monotonic()))[0]:
            chunk = os.read(process.stdout.fileno(), 4096)
            output += chunk
            if b'READY' in output or b'"event":"ready"' in output:
                return output.decode(errors='replace')
            if not chunk and process.poll() is not None:
                break
    raise RuntimeError('Program failed to become ready:\n' + output.decode(errors='replace'))


def finish(process, timeout=15, stop=False, expected=0):
    if stop and process.poll() is None:
        process.terminate()
    output, _ = process.communicate(timeout=timeout)
    if process.returncode != expected:
        raise RuntimeError(f'Child returned {process.returncode}, expected {expected}:\n{output}')
    return output


def sender(count=3, expect_timeout=False, malformed=False):
    arguments = [sys.executable, ROOT / 'examples/network/udp_sender.py', PEER,
                 '--count', count, '--timeout', '0.25', '--interval', '0.02']
    if expect_timeout:
        arguments.append('--expect-timeout')
    if malformed:
        arguments.append('--malformed')
    output = run(arguments).stdout
    records = [json.loads(line) for line in output.splitlines() if line.startswith('{')]
    summary = records[-1]
    if summary['event'] != 'summary':
        raise RuntimeError('Sender did not produce a summary')
    return summary


def assert_detached():
    links = json.loads(run(inside(['ip', '-j', '-details', 'link', 'show', 'dev', INTERFACE])).stdout)
    state = links[0].get('xdp')
    if state and state.get('attached', 'off') not in ('off', None):
        raise RuntimeError('XDP attachment remains after cleanup: ' + repr(state))


def record(name, details):
    RESULTS.append({'case': name, 'status': 'passed', 'details': details})
    print('PASS ' + name, flush=True)


def checksum(data):
    if len(data) % 2:
        data += b'\0'
    total = sum(struct.unpack('!' + 'H' * (len(data) // 2), data))
    while total >> 16:
        total = (total & 65535) + (total >> 16)
    return (~total) & 65535


def raw_bad_requests():
    host_link = json.loads(run(['ip', '-j', 'link', 'show', 'dev', 'hfs-host']).stdout)[0]
    peer_link = json.loads(run(inside(['ip', '-j', 'link', 'show', 'dev', INTERFACE])).stdout)[0]
    mac = lambda text: bytes.fromhex(text.replace(':', ''))
    ethernet = mac(peer_link['address']) + mac(host_link['address']) + b'\x08\x00'
    source_ip = socket.inet_aton(HOST)
    destination_ip = socket.inet_aton(PEER)
    payload = struct.pack('!3sBI', b'HFS', 1, 999)
    udp = struct.pack('!HHHH', 45000, PORT, 16, 0) + payload
    pseudo = source_ip + destination_ip + struct.pack('!BBH', 0, 17, len(udp))
    udp_checksum = checksum(pseudo + udp) or 65535
    udp = udp[:6] + struct.pack('!H', udp_checksum) + udp[8:]
    ip = struct.pack('!BBHHHBBH4s4s', 0x45, 0, 36, 1, 0, 64, 17, 0,
                     source_ip, destination_ip)
    ip = ip[:10] + struct.pack('!H', checksum(ip)) + ip[12:]
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as observer:
        observer.bind((HOST, 45000))
        observer.settimeout(0.2)
        with socket.socket(socket.AF_PACKET, socket.SOCK_RAW) as raw:
            raw.bind(('hfs-host', 0))
            bad_ip = bytearray(ip)
            bad_ip[10] ^= 1
            bad_udp = bytearray(udp)
            bad_udp[6] ^= 1
            for frame in (ethernet + bad_ip + udp, ethernet + ip + bad_udp):
                raw.send(frame)
                try:
                    reply = observer.recv(2048)
                except socket.timeout:
                    continue
                raise RuntimeError('Responder echoed a bad-checksum request: ' + repr(reply))


def main():
    if platform.system() != 'Linux' or os.geteuid() != 0:
        raise SystemExit('Run with sudo on Linux after building; no privileged tests are skipped.')
    kernel = platform.release()
    created = False
    try:
        run([sys.executable, ROOT / 'scripts/network_lab.py', 'setup'])
        created = True
        run(inside([BUILD / 'xdp_control', INTERFACE, 'self-test', '1']))
        record('kernel BPF verifier and classifier fixtures', 'Pass/drop/unsupported/truncated/missing-XSK cases')

        endpoint = launch('udp_endpoint', PEER, PORT, '3', '8')
        ready(endpoint)
        summary = sender()
        finish(endpoint)
        record('ordinary UDP replies', summary)

        endpoint = launch('udp_endpoint', PEER, PORT, '1', '8')
        ready(endpoint)
        sender(count=1, expect_timeout=True, malformed=True)
        summary = sender(count=1)
        finish(endpoint)
        record('malformed message rejected without terminating endpoint', summary)

        for reader in ('packet_rx', 'packet_ring'):
            endpoint = launch('udp_endpoint', PEER, PORT, '3', '8')
            ready(endpoint)
            capture = launch(reader, INTERFACE, '3', '8')
            ready(capture)
            sender()
            capture_output = finish(capture)
            finish(endpoint)
            record(reader + ' captures numbered requests', capture_output)

        for action in ('pass', 'redirect'):
            endpoint = launch('udp_endpoint', PEER, PORT, '3', '8')
            ready(endpoint)
            controller = launch('xdp_control', INTERFACE, action, '8')
            ready(controller)
            if action == 'pass':
                collision = run(inside([BUILD / 'xdp_control', INTERFACE, 'drop', '1']), check=False)
                if collision.returncode == 0:
                    raise RuntimeError('Existing-program collision was not refused')
            summary = sender()
            output = finish(controller, stop=True)
            finish(endpoint)
            assert_detached()
            record('XDP ' + action + (' missing-XSK fallback' if action == 'redirect' else ' and collision refusal'),
                   {'sender': summary, 'controller': output})

        endpoint = launch('udp_endpoint', PEER, PORT, '1', '12')
        ready(endpoint)
        controller = launch('xdp_control', INTERFACE, 'drop', '8')
        ready(controller)
        sender(count=2, expect_timeout=True)
        # ARP and ICMP do not match the marked UDP flow and must still work.
        run(['ping', '-c', '1', '-W', '2', PEER])
        output = finish(controller, stop=True)
        if 'drop=2' not in output:
            raise RuntimeError('No exact selected-flow drop counter evidence:\n' + output)
        sender(count=1)
        finish(endpoint)
        assert_detached()
        record('selective XDP drop and ordinary path restored', output)

        receiver = launch('xsk_receiver', INTERFACE, '3', '8')
        ready(receiver)
        endpoint = launch('udp_endpoint', PEER, PORT, '1', '12')
        ready(endpoint)
        sender(count=3, expect_timeout=True)
        output = finish(receiver)
        assert_detached()
        sender(count=1)
        finish(endpoint)
        record('AF_XDP copy receive bypasses ordinary UDP socket', output)

        responder = launch('xsk_responder', INTERFACE, '3', '8', '0', '8')
        ready(responder)
        raw_bad_requests()
        summary = sender()
        output = finish(responder)
        assert_detached()
        if 'pending=0' not in output or 'transmitted=3' not in output:
            raise RuntimeError('TX ownership/completion result missing:\n' + output)
        record('AF_XDP reply checksums and small-pool TX completion', {'sender': summary, 'responder': output})

        receiver = launch('xsk_receiver', INTERFACE, '16', '8', '500', '8')
        ready(receiver)
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as burst:
            for sequence in range(1, 65):
                burst.sendto(struct.pack('!3sBI', b'HFS', 1, sequence), (PEER, PORT))
            time.sleep(0.8)
            # Send enough recovery traffic while the receiver consumes/refills.
            for sequence in range(65, 97):
                burst.sendto(struct.pack('!3sBI', b'HFS', 1, sequence), (PEER, PORT))
                time.sleep(0.01)
        output = finish(receiver)
        assert_detached()
        record('bounded FILL starvation and recovery', output)

        # Timeout/error cleanup must detach too, and a second run can reattach.
        receiver = launch('xsk_receiver', INTERFACE, '1', '1')
        ready(receiver)
        output = finish(receiver, expected=1)
        assert_detached()
        responder = launch('xsk_responder', INTERFACE, '1', '5')
        ready(responder)
        sender(count=1)
        finish(responder)
        assert_detached()
        record('timeout cleanup and subsequent reattach', output)
    finally:
        for process in CHILDREN:
            if process.poll() is None:
                process.terminate()
                try:
                    process.communicate(timeout=3)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.communicate()
        if created:
            run([sys.executable, ROOT / 'scripts/network_lab.py', 'cleanup'])
    report = {'status': 'all privileged namespace cases passed', 'kernel': kernel,
              'architecture': platform.machine(), 'cases': RESULTS,
              'environment': 'Linux helper-owned veth/network namespace; generic XDP and AF_XDP copy mode',
              'physical_runtime': 'not exercised'}
    (BUILD / 'runtime-report.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
