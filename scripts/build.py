#!/usr/bin/env python3
"""Stage only independently accepted pages, then build mdBook/Vercel output."""
import argparse, hashlib, json, re, shutil, subprocess, zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'.authoring/stage'
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def accepted(catalog):
    result=[]
    for entry in catalog:
        report=ROOT/'reviews'/f"{entry['id']}-review.json"
        if not report.exists():continue
        review=json.loads(report.read_text())
        if review.get('status')!='accepted_source_review':continue
        source=ROOT/entry['path']
        if review.get('canonical_sha256')!=digest(source):
            raise RuntimeError(f"Review hash missing/stale: {entry['id']}")
        text=source.read_text()
        included={(source.parent/name.strip()).resolve().relative_to(ROOT).as_posix()
                  for name in re.findall(r'\{\{#include\s+([^}]+)\}\}',text)}
        assets={(source.parent/name).resolve().relative_to(ROOT).as_posix()
                for name in re.findall(r'\]\((assets/[^)#?]+\.svg)(?:[?#][^)]*)?\)',text)}
        for field,required in [('included_source_sha256',included),('asset_sha256',assets)]:
            missing=required-set(review.get(field,{}))
            if missing:raise RuntimeError(f"Review omits {field}: {entry['id']}: {sorted(missing)}")
        for name,sha in review.get('included_source_sha256',{}).items():
            if digest(ROOT/name)!=sha:raise RuntimeError(f"Included source changed since review: {entry['id']}: {name}")
        for name,sha in review.get('asset_sha256',{}).items():
            if digest(ROOT/name)!=sha:raise RuntimeError(f"Diagram changed since review: {entry['id']}: {name}")
        result.append(entry)
    return result

def main():
    p=argparse.ArgumentParser();p.add_argument('--partial',action='store_true');args=p.parse_args()
    catalog=json.loads((ROOT/'catalog.json').read_text());published=accepted(catalog)
    if len(published)!=len(catalog) and not args.partial:raise SystemExit(f'Only {len(published)}/{len(catalog)} pages independently accepted; use --partial for a progressive release')
    if STAGE.exists():shutil.rmtree(STAGE)
    (STAGE/'src').mkdir(parents=True)
    shutil.copy2(ROOT/'book.toml',STAGE/'book.toml')
    (STAGE/'src/assets').mkdir()
    reviewed_assets={'src/assets/book.css'}
    for entry in published:
        review=json.loads((ROOT/'reviews'/f"{entry['id']}-review.json").read_text())
        reviewed_assets.update(review.get('asset_sha256',{}))
    for name in sorted(reviewed_assets):
        target=STAGE/name;target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(ROOT/name,target)
    shutil.copytree(ROOT/'examples',STAGE/'examples',ignore=shutil.ignore_patterns('*.o','*.ko','*.mod','*.mod.c','.*.cmd','Module.symvers','modules.order','.tmp_versions'))
    ids={e['id'] for e in published};byname={Path(e['path']).name:e for e in catalog}
    def replace_future(match):
        label,url=match.groups()
        target=url.split('#')[0]
        e=byname.get(target)
        if e and e['id'] not in ids:return f'[{label}](roadmap.html#lesson-{e["id"].lower().replace(".","-")})'
        return match.group(0)
    for entry in published:
        source=ROOT/entry['path'];text=source.read_text()
        text=re.sub(r'\[([^\]]+)\]\(([^)]+)\)',replace_future,text)
        (STAGE/'src'/source.name).write_text(text)
    for name in ['index.md','verification.md']:
        text=(ROOT/'src'/name).read_text()
        if name=='index.md':text+='\n## Reading progress\n\n'+f'**{len(published)} of {len(catalog)} planned lessons are published.** Each page appears after independent review. [See the complete roadmap](roadmap.md) for upcoming lessons.\n'
        (STAGE/'src'/name).write_text(text)
    lines=['# Summary','','[Start here](index.md)','','[Roadmap and reading progress](roadmap.md)','','[Verification and limitations](verification.md)','','[Download the examples](downloads.md)','']
    current=None
    for entry in published:
        if entry['chapter']!=current:current=entry['chapter'];lines+=['',f'# {current}','']
        lines.append(f"- [{entry['id']} — {entry['title']}]({Path(entry['path']).name})")
    (STAGE/'src/SUMMARY.md').write_text('\n'.join(lines)+'\n')
    roadmap=['# Roadmap and reading progress','','This book is being published one independently reviewed page at a time. Upcoming entries are the agreed scope, not completed lessons.','']
    for entry in catalog:
        roadmap += [f'<a id="lesson-{entry["id"].lower().replace(".","-")}"></a>',f'## {entry["id"]} — {entry["title"]}','']
        roadmap += [f'[Read this lesson]({Path(entry["path"]).name}).' if entry['id'] in ids else 'Not yet published.', '',entry['scope'],'']
    (STAGE/'src/roadmap.md').write_text('\n'.join(roadmap)+'\n')
    files=sorted(x for x in (STAGE/'examples').rglob('*') if x.is_file() and (x.suffix in ['.cpp','.hpp','.c','.dts','.rules'] or x.name in ['Makefile','CMakeLists.txt']))
    download='# Download the examples\n\n[Download every example and build script](downloads/hardware-examples.zip). Extract it and work from its root directory. Shared headers are included.\n\nThe full example set is available ahead of some lessons. These programs have compilation/software checks, but physical runtime has not been exercised; use the published lesson and verification matrix before operating hardware.\n\n'
    download+='\n'.join(f'- [{x.relative_to(STAGE)}](downloads/{x.relative_to(STAGE).as_posix()})' for x in files)+'\n'
    (STAGE/'src/downloads.md').write_text(download)
    subprocess.run(['mdbook','build',str(STAGE)],cwd=ROOT,check=True)
    if (ROOT/'book').exists():shutil.rmtree(ROOT/'book')
    shutil.copytree(STAGE/'book',ROOT/'book')
    dest=ROOT/'book/downloads';dest.mkdir(exist_ok=True)
    for x in files:
        target=dest/x.relative_to(STAGE);target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(x,target)
    with zipfile.ZipFile(dest/'hardware-examples.zip','w',zipfile.ZIP_DEFLATED) as z:
        for x in files:z.write(x,x.relative_to(STAGE).as_posix())
        for x in list((ROOT/'scripts').glob('*.py'))+[ROOT/'EXAMPLES.md',ROOT/'README.md']:
            z.write(x,x.relative_to(ROOT).as_posix())
    if (ROOT/'verification.json').exists():shutil.copy2(ROOT/'verification.json',ROOT/'book/verification.json')
    manifest={'published':[e['id'] for e in published],'total':len(catalog),'canonical_sha256':{e['path']:digest(ROOT/e['path']) for e in published}}
    (ROOT/'book/publication.json').write_text(json.dumps(manifest,indent=2)+'\n')
    (ROOT/'.authoring/publication.json').write_text(json.dumps(manifest,indent=2)+'\n')
    output=ROOT/'.vercel/output';output.mkdir(parents=True,exist_ok=True)
    (output/'config.json').write_text('{"version":3}\n')
    if (output/'static').exists():shutil.rmtree(output/'static')
    shutil.copytree(ROOT/'book',output/'static')
    print(f'Built {len(published)}/{len(catalog)} reviewed catalog pages plus start, roadmap, verification and downloads')
if __name__=='__main__':main()
