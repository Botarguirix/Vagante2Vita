"""Show the instructions before each call inside a game function (recover arguments Ghidra lost).

    python tools/call_args.py main                       # every call in main
    python tools/call_args.py main --callee ResourceManager::initialize --before 8

Needs llvm-objdump (MSYS2: mingw-w64-ucrt-x86_64-llvm-tools) and _local/pdb/pseudo_index.json.
"""
import argparse, json, pathlib, re, subprocess

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('function'); ap.add_argument('--callee', action='append', default=[])
    ap.add_argument('--before', type=int, default=6)
    ap.add_argument('--exe', default=r'F:\SteamLibrary\steamapps\common\vagante\vagante.exe')
    ap.add_argument('--objdump', default=r'C:\msys64\ucrt64\bin\llvm-objdump.exe')
    a = ap.parse_args()
    idx = json.loads(pathlib.Path('_local/pdb/pseudo_index.json').read_text())
    fns = json.loads(pathlib.Path('_local/pdb/game_functions.json').read_text())
    by_addr = {int(e['addr'], 16): n for n, es in idx.items() for e in es}
    start = int(idx[a.function][0]['addr'], 16)
    size = fns.get(a.function, {}).get('size') or (min(x for x in by_addr if x > start) - start)
    out = subprocess.run([a.objdump, '--no-show-raw-insn', '-d', '--start-address=%#x' % start,
                          '--stop-address=%#x' % (start + size), a.exe], capture_output=True, text=True).stdout
    lines = [re.sub(r'\s+', ' ', l).strip() for l in out.splitlines() if re.match(r'\s+[0-9a-f]+:', l)]
    for i, l in enumerate(lines):
        m = re.search(r'call\w* 0x([0-9a-f]+)', l)
        if not m: continue
        name = by_addr.get(int(m.group(1), 16), '0x' + m.group(1))
        if a.callee and name not in a.callee: continue
        print('== %s' % name)
        for p in lines[max(0, i - a.before):i + 1]: print('   ' + p)

if __name__ == '__main__': main()
