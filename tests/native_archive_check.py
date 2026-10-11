"""Exercise native inflate buffering using generated PNGs, without game files."""
import pathlib, re, struct, subprocess, sys, tempfile, zlib

def chunk(kind,data):
    return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))

def main():
    names=re.findall(r'\{"(gfx/[^"\n]+)",0,0,0\}',pathlib.Path('vita/src/resources.c').read_text())
    png=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',16,16,8,6,0,0,0))
    png+=chunk(b'IDAT',zlib.compress((b'\0'+bytes([120,180,220,255])*16)*16))+chunk(b'IEND',b'')
    records=[('padding',b'\0'*100000)]+[(name,png) for name in names]
    raw=bytearray(struct.pack('<IH',8,len(records)))
    for name,data in records:
        n=name.encode();raw+=struct.pack('<I',len(n))+n+struct.pack('<I',len(data))+data
    compressed=zlib.compress(raw)
    assert len(raw)>32768 and len(compressed)<32768
    cases={'valid':compressed,'truncated':compressed[:-1],'trailing':compressed+b'X','checksum':compressed[:-1]+bytes([compressed[-1]^1])}
    with tempfile.TemporaryDirectory() as d:
        for name,data in cases.items():
            file=pathlib.Path(d)/(name+'.vra');file.write_bytes(data)
            r=subprocess.run([sys.argv[1],str(file),str(pathlib.Path(d)/'preview.ppm')],capture_output=True)
            expected=0 if name=='valid' else 1
            if r.returncode!=expected:raise AssertionError((name,r.returncode,r.stdout.decode(),r.stderr.decode()))
    print('Native compressed-buffer drain and integrity checks passed')

if __name__=='__main__':main()
