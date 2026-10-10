"""Verify and package locally generated pseudocode. Never publish this archive."""
import csv, hashlib, json, pathlib, zipfile

root=pathlib.Path('_local/decompiled');rows=list(csv.DictReader((root/'functions.tsv').open(encoding='utf-8'),delimiter='\t'))
ok=[r for r in rows if r['status']=='ok'];failed=[r for r in rows if r['status']!='ok']
for row in ok:
    path=root/row['file']
    if path.parent!=root or not path.is_file() or path.stat().st_size==0:raise ValueError('Missing export '+row['address'])
summary={'recognized_internal_functions':len(rows),'exported_pseudocode_functions':len(ok),'remaining_failed_functions':len(failed),'exported_types':sum(1 for _ in (root/'types.tsv').open(encoding='utf-8'))-1,'portable_reconstructed_functions':0,'behavior_verified':False,'notes':'Counts include linked libraries/runtime; pseudocode is not recovered original C++.'}
binary=json.loads(pathlib.Path('_local/analysis/binary.json').read_text())
summary['input_exe_sha256']=binary['sha256'];summary['input_pdb_sha256']=binary['pdb']['sha256'];summary['pdb_matches']=binary['pdb_matches']
(root/'verified_summary.json').write_text(json.dumps(summary,indent=2)+'\n')
(root/'README.txt').write_text('Vagante2Vita local reverse-engineering output\n\nGhidra pseudocode, not original C++ and not verified recompilable source.\nIncludes linked libraries and compiler runtime.\nfunctions.tsv and verified_summary.json describe final export status.\nsummary.txt describes the first pass; retry.tsv describes subsequent retries.\ntypes.tsv and fields.tsv describe observed Windows layouts, not portable headers.\nDo not publish this local archive as the public repository.\nUse the original local EXE/PDB and saved Ghidra project to investigate warnings.\n')
out=pathlib.Path('_local/Vagante-decompilation.zip')
with zipfile.ZipFile(out,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as z:
    names={r['file'] for r in ok}|{'functions.tsv','types.tsv','fields.tsv','summary.txt','retry.tsv','retry_summary.txt','verified_summary.json','README.txt'}
    for name in sorted(names):
        p=root/name
        if p.is_file():z.write(p,'decompiled/'+p.name)
with zipfile.ZipFile(out) as z:
    if z.testzip() is not None:raise ValueError('Damaged ZIP')
with out.open('rb') as f:digest=hashlib.file_digest(f,'sha256').hexdigest()
print(json.dumps(summary,indent=2));print('Local ZIP:',out,'bytes:',out.stat().st_size,'sha256:',digest)
