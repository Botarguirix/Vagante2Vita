"""Inspect/extract a local Vagante archive. Format reference: mtolly/vagante-extract.
Independent implementation; no third-party source copied.
"""
import argparse, collections, hashlib, json, pathlib, struct, zlib

def entries(path, max_bytes=512*1024*1024):
    compressed=path.read_bytes();decoder=zlib.decompressobj()
    data=decoder.decompress(compressed,max_bytes+1)
    if len(data)>max_bytes or decoder.unconsumed_tail:raise ValueError('Archive exceeds decompression limit')
    if not decoder.eof or decoder.unused_data:raise ValueError('Invalid or trailing compressed stream')
    if len(data)<6:raise ValueError('Missing header')
    magic,count=struct.unpack_from('<IH',data);pos=6;files=[]
    while pos<len(data):
        if pos+4>len(data):raise ValueError('Truncated name length')
        n=struct.unpack_from('<I',data,pos)[0];pos+=4
        if n==0 or n>4096 or pos+n+4>len(data):raise ValueError('Invalid name length')
        name=data[pos:pos+n].decode('utf-8');pos+=n
        clean=name.replace('\\','/');p=pathlib.PurePosixPath(clean)
        if p.is_absolute() or ':' in clean or '..' in p.parts or '\0' in name:raise ValueError('Unsafe archive path')
        size=struct.unpack_from('<I',data,pos)[0];pos+=4
        if pos+size>len(data):raise ValueError('Truncated content')
        files.append((clean,data[pos:pos+size]));pos+=size
    return magic,count,files,len(data)

def main():
    a=argparse.ArgumentParser();a.add_argument('archive',type=pathlib.Path);a.add_argument('--out',type=pathlib.Path,default=pathlib.Path('_local/analysis/archive.json'));a.add_argument('--extract',type=pathlib.Path);args=a.parse_args()
    magic,count,files,size=entries(args.archive)
    stats=collections.Counter(pathlib.PurePosixPath(n).suffix.lower() for n,b in files)
    duplicates={n:c for n,c in collections.Counter(n for n,b in files).items() if c>1}
    report=dict(sha256=hashlib.sha256(args.archive.read_bytes()).hexdigest(),compressed_bytes=args.archive.stat().st_size,uncompressed_bytes=size,magic=magic,stated_count=count,actual_count=len(files),count_delta=len(files)-count,duplicates=duplicates,extensions=dict(stats),files=[dict(name=n,size=len(b),sha256=hashlib.sha256(b).hexdigest()) for n,b in files])
    args.out.parent.mkdir(parents=True,exist_ok=True);args.out.write_text(json.dumps(report,indent=2)+'\n')
    if args.extract:
        root=args.extract.resolve()
        if root.exists() and any(root.iterdir()):raise FileExistsError('Extraction directory must be empty')
        root.mkdir(parents=True,exist_ok=True)
        for ordinal,(name,b) in enumerate(files):
            dest=(root/name).resolve()
            if not dest.is_relative_to(root):raise ValueError('Path leaves destination')
            if dest.exists():
                # Preserve repeated records without silently deciding runtime precedence.
                dest=root/'_duplicates'/str(ordinal)/name
                if dest.exists():raise FileExistsError(dest)
            dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(b)
    print(json.dumps({k:v for k,v in report.items() if k!='files'},indent=2))
if __name__=='__main__':main()
