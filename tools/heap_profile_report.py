"""Resolve heap-profile callsites using the private PDB/pseudocode index.

python -I tools/heap_profile_report.py <trace> --out artifacts/heap-profile/report.md
No game code or assets are copied. The index remains in rama A/_local.
"""
import argparse,bisect,json,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('trace',type=Path)
 p.add_argument('--index',type=Path,default=ROOT.parent/'Vagante2Vita/_local/pdb/pseudo_index.json')
 p.add_argument('--out',type=Path,required=True)
 p.add_argument('--periodic',choices=['latest','all','none'],default='latest')
 a=p.parse_args()
 data=json.loads(a.index.read_text(encoding='utf-8'))
 entries=sorted((int(e['addr'],16),name) for name,items in data.items() for e in items)
 addresses=[x[0] for x in entries]
 def resolve(addr):
  if not addr:return 'host directo / sin contexto guest'
  # Native scopes use exact function starts; guest returns use their lower neighbour.
  i=bisect.bisect_right(addresses,addr)-1
  if i<0:return 'sin símbolo'
  base,name=entries[i]
  return f'{name} +0x{addr-base:x} (inicio 0x{base:08x}; vecino inferior)'
 text=a.trace.read_text(encoding='utf-8',errors='replace')
 chunks=re.findall(r'(# HEAP_PROFILE BEGIN .*?# HEAP_PROFILE END[^\n]*)',text,re.S)
 if a.periodic!='all':
  periodic=[c for c in chunks if 'reason=periodic-30s ' in c.splitlines()[0]]
  chunks=[c for c in chunks if 'reason=periodic-30s ' not in c.splitlines()[0] or (a.periodic=='latest' and c is periodic[-1])]
 lines=['# Perfil de heap — rama C','',f'Trace: `{a.trace}`','',
 'Bytes = capacidad viva TLSF; requested = petición sin redondear. Los rangos son',
 'vecinos inferiores por bisect del índice: no equivalen a un stack completo. FUN_ es un nombre generado, sin símbolo PDB resuelto.',
 'Las capturas OOM pueden observar operaciones de imagen aún en curso.','']
 for number,chunk in enumerate(chunks,1):
  head=chunk.splitlines()[0]
  reason=re.search(r'reason=(\S+)',head)[1]
  dump_dir=a.out.parent/'dumps';dump_dir.mkdir(parents=True,exist_ok=True)
  dump_path=dump_dir/f'{number:02d}-{re.sub("[^a-zA-Z0-9_-]","_",reason)}.log'
  dump_path.write_text(chunk+'\n',encoding='utf-8')
  lines += [f'## {number}. {reason}','',f'`{head}`','',
   '| Origen | Función según PDB | Bytes vivos | Bloques |',
   '|---|---|---:|---:|']
  for addr,size,blocks in re.findall(r'ORIGIN rank=\d+ addr=([0-9a-f]+) bytes=(\d+) requested=\d+ blocks=(\d+)',chunk)[:15]:
   name=resolve(int(addr,16)).replace('|','\\|')
   lines.append(f'| 0x{addr} | {name} | {int(size):,} | {blocks} |')
  lines += ['', f'[Volcado completo, 30 orígenes]({dump_path.as_posix()})', '', '```text',*filter(lambda l:' HIST ' in l or ' IMAGES ' in l,chunk.splitlines()),'```','']
 lines += ['## Excepciones observadas','', '```text']
 exceptions=[line for line in text.splitlines() if 'RaiseException(' in line or '# catch:' in line or '# unhandled exception' in line]
 lines += exceptions or ['(ninguna línea coincidente)']
 lines += ['```','','## Entrada al nivel y salida del host','','```text']
 lines += [line for line in text.splitlines() if 'HEAP_PROFILE WORLD ' in line or '# vagante host:' in line or '# exit:' in line]
 lines += ['```','']
 a.out.parent.mkdir(parents=True,exist_ok=True)
 a.out.write_text('\n'.join(lines),encoding='utf-8')
 print(f'{len(chunks)} snapshots -> {a.out}')
if __name__=='__main__':main()
