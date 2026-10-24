"""List the stores a game method makes into `this`, named with the reconstructed header layout.

    python tools/field_writes.py Client::Client
    python tools/field_writes.py Client::Client --class Client --all   # also non-immediate stores

Ghidra's pseudocode can show wrong member names when it analysed a stale class definition;
this reads the real instructions (llvm-objdump) and resolves each offset against
recon/include/<Class>.h, where every member carries its verified "// +0xNN" offset.
"""
import argparse, json, pathlib, re, struct, subprocess

def members(header, cls):
    """[(offset, name, type)] of the top-level members of cls, from the generated header."""
    out, depth, inside = [], 0, False
    for line in pathlib.Path(header).read_text(encoding='utf-8').splitlines():
        if re.match(r'^(class|struct|union) %s\b' % re.escape(cls), line): inside = True; depth = 0
        if not inside: continue
        depth += line.count('{') - line.count('}')
        m = re.match(r'^    (?!\s)(.+?)\s+&?\*?(\w+)(\[\d+\])?;  // \+0x([0-9A-F]+)$', line)
        if m and depth == 1: out.append((int(m.group(4), 16), m.group(2), m.group(1)))
        if inside and depth <= 0 and '}' in line: break
    return sorted(out)

def resolve(fields, off):
    best = None
    for o, name, typ in fields:
        if o <= off: best = (o, name, typ)
    if not best: return '+0x%X' % off
    return best[1] if best[0] == off else '%s+0x%X' % (best[1], off - best[0])

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('function'); ap.add_argument('--class', dest='cls')
    ap.add_argument('--all', action='store_true', help='also list stores of registers')
    ap.add_argument('--exe', default=r'F:\SteamLibrary\steamapps\common\vagante\vagante.exe')
    ap.add_argument('--objdump', default=r'C:\msys64\ucrt64\bin\llvm-objdump.exe')
    a = ap.parse_args()
    cls = a.cls or a.function.split('::')[0]
    fields = members('recon/include/%s.h' % cls, cls)
    idx = json.loads(pathlib.Path('_local/pdb/pseudo_index.json').read_text())
    fns = json.loads(pathlib.Path('_local/pdb/game_functions.json').read_text())
    start = int(idx[a.function][0]['addr'], 16); size = fns[a.function]['size']
    by_addr = {int(e['addr'], 16): n for n, es in idx.items() for e in es}
    asm = subprocess.run([a.objdump, '--no-show-raw-insn', '-d', '--start-address=%#x' % start,
                          '--stop-address=%#x' % (start + size), a.exe], capture_output=True, text=True).stdout
    this = {'ecx': 0}   # register -> offset from `this` it currently holds
    slots = {}          # stack slot (ebp offset) -> offset from `this` stored there
    num = lambda s: int(s, 16) if s.startswith(('0x', '-0x')) else int(s or 0)
    for line in asm.splitlines():
        m = re.match(r'\s+([0-9a-f]+):\s+(\S+)\s*(.*)$', line)
        if not m: continue
        addr, op, args = m.groups()
        call = re.search(r'call\w*\s+0x([0-9a-f]+)', line)
        if call:
            name = by_addr.get(int(call.group(1), 16), call.group(1))
            ecx = this.get('ecx')
            print('%s  call %s%s' % (addr, name, ('   [ecx = &%s]' % resolve(fields, ecx)) if ecx is not None else ''))
            for r in ('eax', 'ecx', 'edx'): this.pop(r, None)
            continue
        mv = re.match(r'%(\w+), %(\w+)$', args)
        ld = re.match(r'(-?0x[0-9a-f]+|\d*)\(%(\w+)\), %(\w+)$', args)
        spill = re.match(r'%(\w+), (-?0x[0-9a-f]+)\(%ebp\)$', args)
        if op == 'movl' and spill and spill.group(1) in this:     # this saved on the stack
            slots[spill.group(2)] = this[spill.group(1)]
        if op == 'movl' and ld and ld.group(2) == 'ebp' and ld.group(1) in slots:  # reloaded
            this[ld.group(3)] = slots[ld.group(1)]
            continue
        if op == 'movl' and mv:
            if mv.group(1) in this: this[mv.group(2)] = this[mv.group(1)]
            else: this.pop(mv.group(2), None)
            continue
        if op == 'leal' and ld:
            if ld.group(2) in this: this[ld.group(3)] = this[ld.group(2)] + num(ld.group(1))
            else: this.pop(ld.group(3), None)
            continue
        dst = re.search(r'%(\w+)$', args)
        if dst and not op.startswith(('cmp', 'test', 'push')):
            reg = dst.group(1)
            for full, parts in (('eax', ('ax', 'al')), ('ecx', ('cx', 'cl')), ('edx', ('dx', 'dl')), ('ebx', ('bx', 'bl'))):
                if reg in parts: reg = full
            this.pop(reg, None)
        st = re.match(r'(\$-?0x[0-9a-f]+|\$-?\d+|%\w+), (-?0x[0-9a-f]+|\d*)\(%(\w+)\)$', args)
        if op.startswith('mov') and st and st.group(3) in this:
            off = this[st.group(3)] + num(st.group(2))
            src = st.group(1)
            if not src.startswith('$') and not a.all: continue
            val = src
            if src.startswith('$'):
                v = int(src[1:], 16) if 'x' in src else int(src[1:])
                v &= 0xffffffff
                f = struct.unpack('<f', struct.pack('<I', v))[0]
                val = '%s (%d%s)' % (src, v if v < 0x80000000 else v - (1 << 32),
                                     ', %gf' % f if 0x3a000000 < v < 0x50000000 or 0xba000000 < v < 0xd0000000 else '')
            print('%s  %-6s %-40s = %s' % (addr, op, resolve(fields, off), val))

if __name__ == '__main__':
    main()
