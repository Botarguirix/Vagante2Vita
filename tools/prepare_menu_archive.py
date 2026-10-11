"""Make a private menu-only VRA from the user's original data.vra."""
import argparse, hashlib, json, pathlib, re, struct, zlib
from vra import entries

def main():
    p=argparse.ArgumentParser()
    p.add_argument('archive',type=pathlib.Path)
    p.add_argument('--out',type=pathlib.Path,default=pathlib.Path('_local/menu-only.vra'))
    args=p.parse_args()
    if args.out.exists():raise FileExistsError(args.out)
    source=pathlib.Path(__file__).resolve().parent.parent/'vita/src/resources.c'
    names=re.findall(r'\{"(gfx/[^"\n]+)",0,0,0\}',source.read_text())
    magic,declared,files,total=entries(args.archive)
    if magic!=8:raise ValueError('Unsupported archive magic')
    selected={}
    for name,content in files:
        if name in names:
            if name in selected:raise ValueError('Duplicate required resource: '+name)
            selected[name]=content
    if len(selected)!=len(names):raise ValueError('Missing required menu resources')
    raw=bytearray(struct.pack('<IH',8,len(names)))
    for name in names:
        encoded=name.encode();content=selected[name]
        raw+=struct.pack('<I',len(encoded))+encoded+struct.pack('<I',len(content))+content
    packed=zlib.compress(raw,6)
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_bytes(packed)
    report=dict(source_sha256=hashlib.sha256(args.archive.read_bytes()).hexdigest(),source_records=len(files),records=len(names),compressed_bytes=len(packed),uncompressed_bytes=len(raw),sha256=hashlib.sha256(packed).hexdigest(),resources=names)
    args.out.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='resources'},indent=2))

if __name__=='__main__':main()
