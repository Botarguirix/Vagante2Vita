"""Resolve f_XXXXXXXX frames in a native GDB backtrace using the private PDB index."""
import argparse,bisect,json,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('input',type=Path);p.add_argument('--out',type=Path,required=True)
 p.add_argument('--index',type=Path,default=ROOT.parent/'Vagante2Vita/_local/pdb/pseudo_index.json');a=p.parse_args()
 d=json.loads(a.index.read_text(encoding='utf-8'));e=sorted((int(x['addr'],16),n) for n,v in d.items() for x in v);addresses=[x[0] for x in e]
 def resolve(m):
  address=int(m[1],16);i=bisect.bisect_right(addresses,address)-1
  if i<0:return m[0]+' [no symbol]'
  start,name=e[i];return m[0]+f' [{name}+0x{address-start:x}; lower-neighbour]'
 a.out.write_text(re.sub(r'\bf_([0-9a-fA-F]{8})\b',resolve,a.input.read_text(encoding='utf-8',errors='replace')),encoding='utf-8')
 print(a.out)
if __name__=='__main__':main()
