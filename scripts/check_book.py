#!/usr/bin/env python3
"""Check complete coverage, includes, rendered local links/anchors and downloads."""
import argparse, hashlib, html.parser, json, re, urllib.parse, xml.etree.ElementTree as ET
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class Page(html.parser.HTMLParser):
    def __init__(self):super().__init__(convert_charrefs=True);self.ids=set();self.links=[];self.images=[];self.code=[];self.current=None
    def handle_starttag(self,tag,attrs):
        a=dict(attrs)
        if a.get('id'):self.ids.add(a['id'])
        if tag=='a' and a.get('href'):self.links.append(a['href'])
        if tag=='img' and a.get('src'):self.images.append(a['src'])
        if tag=='code':self.current=''
    def handle_data(self,data):
        if self.current is not None:self.current+=data
    def handle_endtag(self,tag):
        if tag=='code' and self.current is not None:self.code.append(self.current);self.current=None

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--partial',action='store_true');a=ap.parse_args()
    catalog=json.loads((ROOT/'catalog.json').read_text());errors=[];checks=0;includes=0
    if a.partial:
        ids=set(json.loads((ROOT/'book/publication.json').read_text())['published'])
        catalog=[e for e in catalog if e['id'] in ids]
    pages={}
    for f in (ROOT/'book').rglob('*.html'):
        p=Page();p.feed(f.read_text());pages[f.resolve()]=p
    for e in catalog:
        f=ROOT/e['path']
        if not f.exists():
            if not a.partial:errors.append('Missing '+e['path'])
            continue
        s=f.read_text();out=(ROOT/'book'/f.with_suffix('.html').name).resolve()
        if not s.startswith('# '):errors.append('Missing title '+str(f))
        for inc in re.findall(r'\{\{#include\s+([^}]+)\}\}',s):
            # This book includes full files, not line/anchor ranges.
            source=(f.parent/inc.strip()).resolve()
            if not source.is_file():errors.append('Missing include '+inc+' in '+e['id']);continue
            includes+=1
            if out in pages and source.read_text().rstrip('\n') not in [x.rstrip('\n') for x in pages[out].code]:
                errors.append('Rendered include differs '+inc+' in '+e['id'])
        if out not in pages:errors.append('Missing rendered '+str(out))
    for f,p in pages.items():
        for url in p.links+p.images:
            u=urllib.parse.urlsplit(url)
            if u.scheme or u.netloc or url.startswith('javascript:'):continue
            target=(f.parent/urllib.parse.unquote(u.path)).resolve() if u.path else f
            if target.is_dir():target=target/'index.html'
            checks+=1
            if not target.exists():errors.append(f'{f.name}: missing {url}')
            elif u.fragment and target in pages and urllib.parse.unquote(u.fragment) not in pages[target].ids:
                errors.append(f'{f.name}: missing anchor {url}')
    for f in (ROOT/'src/assets').glob('*.svg'):
        try:ET.parse(f)
        except ET.ParseError as e:errors.append(f'{f}: {e}')
    report={'catalog_pages':sum((ROOT/e['path']).exists() for e in catalog),'rendered_html':len(pages),'local_link_checks':checks,'exact_includes_checked':includes,'errors':errors}
    (ROOT/'.authoring').mkdir(exist_ok=True)
    (ROOT/'.authoring/book-check.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2));raise SystemExit(bool(errors))
if __name__=='__main__':main()
