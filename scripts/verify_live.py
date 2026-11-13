#!/usr/bin/env python3
"""Anonymous deployed-output verification. No Vercel auth or bypass token."""
import hashlib,json,urllib.request
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE='https://hardware-from-software.vercel.app/'
def main():
    manifest=json.loads((ROOT/'book/publication.json').read_text())
    paths=['index.html','roadmap.html','verification.html','downloads.html','publication.json','verification.json','downloads/hardware-examples.zip']
    catalog=json.loads((ROOT/'catalog.json').read_text())
    paths += [Path(e['path']).with_suffix('.html').name for e in catalog if e['id'] in manifest['published']]
    paths += [p.relative_to(ROOT/'book').as_posix() for p in (ROOT/'book/assets').glob('*.svg')]
    paths += [p.name for p in (ROOT/'book').glob('search*.js')]
    paths += [p.relative_to(ROOT/'book').as_posix() for p in (ROOT/'book/css').glob('*.css')]
    paths += [p.relative_to(ROOT/'book').as_posix() for p in (ROOT/'book/downloads').rglob('*') if p.is_file()]
    paths=list(dict.fromkeys(paths))
    records=[]
    for name in paths:
        target=BASE+name+'?sha256='+hashlib.sha256((ROOT/'book'/name).read_bytes()).hexdigest()
        with urllib.request.urlopen(urllib.request.Request(target,headers={'User-Agent':'HardwareBookAnonymousCheck/1.0'}),timeout=30) as r:
            assert r.status==200 and r.url==target,(target,r.status,r.url)
            data=r.read()
        local=(ROOT/'book'/name).read_bytes()
        assert data==local,'Deployed bytes differ: '+name
        records.append({'path':name,'sha256':hashlib.sha256(data).hexdigest(),'status':200})
    result={'public_access':'verified without login, cookie, token or bypass header','published_pages':manifest['published'],'checks':records,'status':'pass'}
    (ROOT/'reviews/publication-check.json').write_text(json.dumps(result,indent=2)+'\n')
    print('Anonymous production checks passed:',len(records),'resources;',len(manifest['published']),'catalog pages')
if __name__=='__main__':main()
