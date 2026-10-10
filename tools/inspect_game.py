"""Read-only PE/MSF inventory. Outputs derived metadata, never original binaries."""
import argparse, collections, hashlib, json, pathlib, struct

def u16(b, p): return struct.unpack_from('<H', b, p)[0]
def u32(b, p): return struct.unpack_from('<I', b, p)[0]
def cstr(b, p): return b[p:b.index(b'\0', p)].decode('utf-8', 'replace')

def pe(path):
    b = path.read_bytes(); p = u32(b, 0x3c)
    if b[:2] != b'MZ' or b[p:p+4] != b'PE\0\0': raise ValueError('Not PE')
    opt = p+24; magic = u16(b,opt); base = u32(b,opt+28) if magic == 0x10b else struct.unpack_from('<Q',b,opt+24)[0]
    sections=[]
    for i in range(u16(b,p+6)):
        s=opt+u16(b,p+20)+i*40
        sections.append(dict(name=b[s:s+8].rstrip(b'\0').decode(),virtual_size=u32(b,s+8),rva=u32(b,s+12),raw_size=u32(b,s+16),offset=u32(b,s+20)))
    def offset(rva):
        for s in sections:
            if s['rva'] <= rva < s['rva']+max(s['virtual_size'],s['raw_size']): return s['offset']+rva-s['rva']
        raise ValueError('Unmapped RVA')
    imports=[]; dd=opt+(96 if magic==0x10b else 112); rva=u32(b,dd+8)
    if rva:
        d=offset(rva)
        while any(b[d:d+20]):
            thunk=u32(b,d) or u32(b,d+16); names=[]; t=offset(thunk); width=4 if magic==0x10b else 8
            while True:
                v=int.from_bytes(b[t:t+width],'little'); t+=width
                if not v: break
                names.append('#'+str(v & 0xffff) if v & (1 << (width*8-1)) else cstr(b,offset(v)+2))
            imports.append(dict(dll=cstr(b,offset(u32(b,d+12))),symbols=names));d+=20
    debug=[]; dr=u32(b,dd+6*8); ds=u32(b,dd+6*8+4)
    if dr:
        for d in range(offset(dr),offset(dr)+ds,28):
            if u32(b,d+12)==2:
                q=u32(b,d+24)
                if b[q:q+4]==b'RSDS':debug.append(dict(guid=b[q+4:q+20].hex(),age=u32(b,q+20),pdb=pathlib.PureWindowsPath(cstr(b,q+24)).name))
    return dict(file=path.name,size=len(b),sha256=hashlib.sha256(b).hexdigest(),machine=hex(u16(b,p+4)),bits=32 if magic==0x10b else 64,image_base=hex(base),entry_rva=hex(u32(b,opt+16)),sections=sections,imports=imports,codeview=debug)

def pdb(path):
    b=path.read_bytes()
    if not b.startswith(b'Microsoft C/C++ MSF 7.00'):raise ValueError('Not MSF 7')
    page=u32(b,32);size=u32(b,44);blockmap=u32(b,52)
    pages=[u32(b,blockmap*page+i*4) for i in range((size+page-1)//page)]
    directory=b''.join(b[p*page:(p+1)*page] for p in pages)[:size];n=u32(directory,0)
    sizes=[u32(directory,4+4*i) for i in range(n)];at=4+4*n;streams=[]
    for size in sizes:
        count=0 if size==0xffffffff else (size+page-1)//page
        pages=[u32(directory,at+4*i) for i in range(count)];at+=4*count
        streams.append(b''.join(b[p*page:(p+1)*page] for p in pages)[:size] if count else b'')
    dbi=streams[3];symstream=u16(dbi,20);syms=streams[symstream];pos=0;public=[]
    while pos+4<=len(syms):
        length=u16(syms,pos);kind=u16(syms,pos+2);end=pos+2+length
        if length<2 or end>len(syms):break
        if kind==0x110e and length>=13:
            public.append(dict(name=cstr(syms,pos+14),section=u16(syms,pos+12),offset=u32(syms,pos+8),flags=u32(syms,pos+4)))
        pos=end
    info=streams[1]
    return dict(file=path.name,size=len(b),sha256=hashlib.sha256(b).hexdigest(),guid=info[12:28].hex(),age=u32(info,8),stream_count=n,symbol_stream=symstream,public_symbol_count=len(public)),public

def main():
    a=argparse.ArgumentParser();a.add_argument('game',type=pathlib.Path);a.add_argument('--out',type=pathlib.Path,default=pathlib.Path('_local/analysis'));args=a.parse_args();args.out.mkdir(parents=True,exist_ok=True)
    report=pe(args.game/'vagante.exe');meta,symbols=pdb(args.game/'vagante.pdb');report['pdb']=meta
    report['pdb_matches']=any(x['guid']==meta['guid'] and x['age']==meta['age'] for x in report['codeview'])
    (args.out/'binary.json').write_text(json.dumps(report,indent=2)+'\n')
    (args.out/'symbols.json').write_text(json.dumps(symbols,indent=2)+'\n')
    print(json.dumps(report,indent=2));print('Symbol examples:',[x['name'] for x in symbols if any(v in x['name'] for v in ['Player','World','sf@','main','Archive'])][:35])
if __name__=='__main__':main()
