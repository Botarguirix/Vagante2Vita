"""Build a local starting-point inventory from PDB-derived public symbols."""
import argparse, collections, json, pathlib

def main():
    a=argparse.ArgumentParser();a.add_argument('--analysis',type=pathlib.Path,default=pathlib.Path('_local/analysis'));args=a.parse_args()
    report=json.loads((args.analysis/'binary.json').read_text());symbols=json.loads((args.analysis/'symbols.json').read_text());base=int(report['image_base'],16)
    if not report.get('pdb_matches'):raise ValueError('EXE/PDB mismatch; symbol addresses must not be used')
    groups=['Player','Entity','Level','ResourceManager','GameSave','Input','Game','Client','Server']
    counts=collections.Counter();rows=['subsystem\taddress\tsymbol']
    for sym in symbols:
        if not(sym['flags']&2) or not(0<sym['section']<=len(report['sections'])):continue
        addr=base+report['sections'][sym['section']-1]['rva']+sym['offset']
        for group in groups:
            if '@'+group+'@@' in sym['name']:
                counts[group]+=1;rows.append(group+'\t'+hex(addr)+'\t'+sym['name'])
        if sym['name'] in ['main','_main','WinMain','_WinMain@16']:rows.append('entry\t'+hex(addr)+'\t'+sym['name'])
    (args.analysis/'triage.tsv').write_text('\n'.join(rows)+'\n')
    print(json.dumps(dict(counts),indent=2))
if __name__=='__main__':main()
