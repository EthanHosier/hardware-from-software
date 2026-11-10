#!/usr/bin/env python3
"""Publish only reviewed pages; call immediately after each page acceptance."""
import datetime, fcntl, hashlib, json, subprocess, time, urllib.request
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
    lock=(ROOT/'.authoring/publish.lock').open('w')
    fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
    subprocess.run(['python3','scripts/build.py','--partial'],cwd=ROOT,check=True)
    subprocess.run(['python3','scripts/check_book.py','--partial'],cwd=ROOT,check=True)
    expected=json.loads((ROOT/'book/publication.json').read_text())
    command=['vercel','deploy','--prebuilt','--prod','--yes','--no-wait']
    result=subprocess.run(command,cwd=ROOT,check=True,capture_output=True,text=True,timeout=120)
    # Current CLI emits structured JSON after progress lines. Save full output ignored.
    (ROOT/'.authoring/last-deploy.log').write_text(result.stdout+'\n'+result.stderr)
    decoder=json.JSONDecoder();deployment=None
    for n,char in enumerate(result.stdout):
        if char=='{':
            try:
                value,_=decoder.raw_decode(result.stdout[n:])
                if isinstance(value,dict) and value.get('status')=='ok' and 'deployment' in value:
                    deployment=value['deployment'];break
            except json.JSONDecodeError:pass
    if deployment is None:raise RuntimeError('Missing structured deployment response; inspect .authoring/last-deploy.log before any retry')
    for attempt in range(40):
        state=json.loads(subprocess.check_output(['vercel','api','/v13/deployments/'+deployment['id'],'--raw'],cwd=ROOT,text=True,timeout=30))
        if state.get('readyState')=='READY':break
        if state.get('readyState') in ['BLOCKED','ERROR','CANCELED']:
            raise RuntimeError('Deployment '+state['readyState']+': '+str(state.get('readyStateReason',state.get('errorMessage'))))
        time.sleep(3)
    else:raise RuntimeError('Deployment pending after bounded wait; inspect existing ID before retry')
    url=deployment.get('productionUrl') or 'https://hardware-from-software.vercel.app'
    # This deliberately uses urllib, without Vercel credentials or bypass headers.
    request=urllib.request.Request(url+'/publication.json',headers={'User-Agent':'HardwareBookPublicVerification/1.0'})
    with urllib.request.urlopen(request,timeout=30) as response:
        if response.url!=url+'/publication.json':raise RuntimeError('Unexpected redirect: '+response.url)
        actual=json.loads(response.read())
    if actual!=expected:raise RuntimeError('Public production manifest differs from this build')
    for entry in json.loads((ROOT/'catalog.json').read_text()):
        if entry['id'] not in expected['published']:continue
        target=url+'/'+Path(entry['path']).with_suffix('.html').name
        with urllib.request.urlopen(target,timeout=30) as response:
            if response.url!=target or response.status!=200:raise RuntimeError('Public chapter verification failed '+target)
            body=response.read()
        local=(ROOT/'book'/Path(entry['path']).with_suffix('.html').name).read_bytes()
        if body!=local:raise RuntimeError('Deployed chapter differs '+target)
    ledger=ROOT/'releases.json'
    releases=json.loads(ledger.read_text()) if ledger.exists() else []
    releases.append({'published_at':datetime.datetime.now(datetime.timezone.utc).isoformat(),'deployment':deployment,'publication':expected,'anonymous_verification':'manifest and every published chapter fetched without authentication and matched local output'})
    ledger.write_text(json.dumps(releases,indent=2)+'\n')
    print(json.dumps({'production':url,'published':expected['published'],'deployment':deployment['id'],'anonymous_verification':'passed'},indent=2))
if __name__=='__main__':main()
