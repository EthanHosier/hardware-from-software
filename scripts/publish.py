#!/usr/bin/env python3
"""Publish only reviewed pages; call immediately after each page acceptance."""
import argparse, datetime, fcntl, hashlib, json, subprocess, time, urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--complete', action='store_true',
                        help='Require every catalog page and run complete book checks')
    args = parser.parse_args()
    lock = (ROOT / '.authoring/publish.lock').open('w')
    fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    scope = [] if args.complete else ['--partial']
    subprocess.run(['python3', 'scripts/build.py', *scope], cwd=ROOT, check=True)
    subprocess.run(['python3', 'scripts/check_book.py', *scope], cwd=ROOT, check=True)
    expected = json.loads((ROOT / 'book/publication.json').read_text())
    command = ['vercel', 'deploy', '--prebuilt', '--prod', '--yes', '--no-wait', '--scope', 'ethanhosiers-projects']
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=120)
    # Current CLI emits structured JSON after progress lines. Save full output ignored.
    (ROOT / '.authoring/last-deploy.log').write_text(result.stdout + '\n' + result.stderr)
    if result.returncode:
        raise RuntimeError('Vercel CLI failed (exit ' + str(
            result.returncode) + '); details saved in .authoring/last-deploy.log: ' + result.stderr[-1800:])
    decoder = json.JSONDecoder();
    deployment = None
    for n, char in enumerate(result.stdout):
        if char == '{':
            try:
                value, _ = decoder.raw_decode(result.stdout[n:])
                if isinstance(value, dict) and value.get('status') == 'ok' and 'deployment' in value:
                    deployment = value['deployment'];
                    break
            except json.JSONDecodeError:
                pass
    if deployment is None: raise RuntimeError(
        'Missing structured deployment response; inspect .authoring/last-deploy.log before any retry')
    for attempt in range(40):
        state = json.loads(
            subprocess.check_output(['vercel', 'api', '/v13/deployments/' + deployment['id'], '--raw'], cwd=ROOT,
                                    text=True, timeout=30))
        if state.get('readyState') == 'READY': break
        if state.get('readyState') in ['BLOCKED', 'ERROR', 'CANCELED']:
            raise RuntimeError('Deployment ' + state['readyState'] + ': ' + str(
                state.get('readyStateReason', state.get('errorMessage'))))
        time.sleep(3)
    else:
        raise RuntimeError('Deployment pending after bounded wait; inspect existing ID before retry')
    url = deployment.get('productionUrl') or 'https://hardware-from-software.vercel.app'
    # This deliberately uses urllib, without Vercel credentials or bypass headers.
    # READY can precede propagation of the production alias. Verify that alias
    # with bounded retries instead of deploying a duplicate on a stale response.
    for attempt in range(20):
        request = urllib.request.Request(url + '/publication.json?release=' + deployment['id'], headers={
            'User-Agent': 'HardwareBookPublicVerification/1.0', 'Cache-Control': 'no-cache'})
        with urllib.request.urlopen(request, timeout=30) as response:
            if response.url != url + '/publication.json?release=' + deployment['id']: raise RuntimeError(
                'Unexpected redirect: ' + response.url)
            actual = json.loads(response.read())
        if actual == expected: break
        time.sleep(2)
    else:
        raise RuntimeError('Public production manifest differs after bounded alias propagation wait')
    for entry in json.loads((ROOT / 'catalog.json').read_text()):
        if entry['id'] not in expected['published']: continue
        target = url + '/' + Path(entry['path']).with_suffix('.html').name + '?release=' + deployment['id']
        with urllib.request.urlopen(target, timeout=30) as response:
            if response.url != target or response.status != 200: raise RuntimeError(
                'Public chapter verification failed ' + target)
            body = response.read()
        local = (ROOT / 'book' / Path(entry['path']).with_suffix('.html').name).read_bytes()
        if body != local: raise RuntimeError('Deployed chapter differs ' + target)
    ledger = ROOT / 'releases.json'
    releases = json.loads(ledger.read_text()) if ledger.exists() else []
    releases.append({'published_at': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'deployment': deployment,
                     'publication': expected,
                     'anonymous_verification': 'manifest and every published chapter fetched without authentication and matched local output'})
    ledger.write_text(json.dumps(releases, indent=2) + '\n')
    print(json.dumps({'production': url, 'published': expected['published'], 'deployment': deployment['id'],
                      'anonymous_verification': 'passed'}, indent=2))


if __name__ == '__main__': main()
