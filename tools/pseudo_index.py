"""Index Ghidra pseudocode files (one function per file) by function name.

    python tools/pseudo_index.py <decompiled dir>            # build _local/pdb/pseudo_index.json
    python tools/pseudo_index.py <decompiled dir> --show GameEngine::run
    python tools/pseudo_index.py <decompiled dir> --unit GameEngine   # list a unit's functions

The pseudocode stays in the user's local folder; only names and paths are indexed.
"""
import argparse, json, pathlib, re

HEAD = re.compile(r'/\* Ghidra pseudocode: (.+) @ ([0-9a-fA-F]+)\.')

def build(src):
    idx = {}
    for p in sorted(src.glob('*.c')):
        with p.open(encoding='utf-8', errors='replace') as f: first = f.readline()
        m = HEAD.match(first)
        if m: idx.setdefault(m.group(1), []).append(dict(addr=m.group(2), file=p.name))
    return idx

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('src', type=pathlib.Path)
    ap.add_argument('--index', type=pathlib.Path, default=pathlib.Path('_local/pdb/pseudo_index.json'))
    ap.add_argument('--show'); ap.add_argument('--unit')
    a = ap.parse_args()
    if not a.index.exists() or not (a.show or a.unit):
        idx = build(a.src); a.index.write_text(json.dumps(idx, indent=0)); print('indexed', len(idx), 'names')
    idx = json.loads(a.index.read_text())
    if a.show:
        for e in idx.get(a.show, []): print((a.src / e['file']).read_text(encoding='utf-8', errors='replace'))
    if a.unit:
        fns = json.loads(pathlib.Path('_local/pdb/game_functions.json').read_text())
        for n, f in sorted(fns.items()):
            if f['unit'] == a.unit:
                print('%6d  %-60s %s' % (f['size'], n, 'pseudo' if n in idx else '-'))

if __name__ == '__main__': main()
