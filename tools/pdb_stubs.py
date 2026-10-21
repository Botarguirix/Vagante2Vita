"""Generate link stubs, free-function declarations and static data for the reconstruction.

    python tools/pdb_stubs.py <vagante>/vagante.pdb <vagante>/vagante.exe

Writes (all regenerated, not versioned):
  recon/include/free_functions.h   declarations of the game's non-member functions
  recon/include/recon_all.h        all.h + free_functions.h (precompiled header)
  recon/stubs/<unit>.stubs.cpp     one stub per function not yet defined in recon/src
  recon/stubs/statics.cpp          every static data member, with the value stored in vagante.exe

A function counts as translated when recon/src/*.cpp contains an unindented
definition of its qualified name; then all its overloads stop being stubbed.
"""
import argparse, collections, math, pathlib, re, struct, sys
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import pdb_headers as H
from pdb_inventory import u16, u32, cstr

# ---------------------------------------------------------------- executable image

class PE:
    def __init__(self, path):
        b = pathlib.Path(path).read_bytes(); p = u32(b, 0x3c); opt = p + 24
        self.b, self.base, self.sec = b, u32(b, opt + 28), []
        for i in range(u16(b, p + 6)):
            s = opt + u16(b, p + 20) + i * 40
            self.sec.append(dict(name=b[s:s + 8].rstrip(b'\0').decode(), rva=u32(b, s + 12), vsize=u32(b, s + 8),
                                 raw=u32(b, s + 20), rsize=u32(b, s + 16), flags=u32(b, s + 36)))

    def read(self, rva, n):
        for s in self.sec:
            if s['rva'] <= rva < s['rva'] + max(s['vsize'], s['rsize']):
                o = rva - s['rva']
                data = self.b[s['raw'] + o:s['raw'] + min(o + n, s['rsize'])] if o < s['rsize'] else b''
                return data + b'\0' * (n - len(data)), s
        return None, None

    def cstring(self, va):
        data, s = self.read(va - self.base, 1)
        if s is None: return None
        o = s['raw'] + va - self.base - s['rva']
        return self.b[o:self.b.index(b'\0', o)]

def c_string(bs):
    out = ''.join(chr(c) if 32 <= c < 127 and chr(c) not in '"\\?' else '\\%03o' % c for c in bs)
    return '"' + out + '"'

def float_lit(v, suffix):
    if math.isnan(v): return 'std::numeric_limits<%s>::quiet_NaN()' % ('float' if suffix else 'double')
    if math.isinf(v): return ('-' if v < 0 else '') + 'std::numeric_limits<%s>::infinity()' % ('float' if suffix else 'double')
    s = ('%.9g' if suffix else '%.17g') % v
    if not re.search(r'[.e]', s): s += '.0'
    return s + suffix

# Third-party sources compiled into the game (imgui): their free functions are not game code.
TOOL_UNITS = {'imgui', 'imgui_draw', 'imgui_demo', 'imgui-SFML'}
# MSVC compiles inline CRT/SDK functions into every object that uses them. They must never be
# stubbed: a stub named fprintf replaces the real one (and deadlocks the stub logger).
NOT_GAME_FUNCTIONS = re.compile(
    r'^(_*v?(s|sn|f|sw|w|fw)?printf(_s|_l|_p)?|_*v?(s|f|sw|w)?scanf(_s|_l)?|'
    r'(a?cos|a?sin|a?tan|atan2|ceil|floor|fabs|fmod|ldexp|frexp|log|log10|exp|pow|sqrt|round|trunc|hypot)f?|'
    r'abs|labs|llabs|IsWindows\w*|SteamInternal_\w+|SteamAPI_\w+|SteamGameServer_\w+)$')

INT_FMT = {(1, True): '<b', (1, False): '<B', (2, True): '<h', (2, False): '<H', (4, True): '<i', (4, False): '<I',
           (8, True): '<q', (8, False): '<Q'}

def literal(db, ti, raw, pe, depth=0):
    """C++ initializer for a value of type ti stored as raw bytes, or None when not expressible."""
    if depth > 8 or raw is None: return None
    if ti < 0x1000:
        t = ti & 0xff
        if (ti >> 8) & 7:
            v = u32(raw, 0)
            if v == 0: return 'nullptr'
            if t in (0x70, 0x10, 0x20):
                s = pe.cstring(v)
                return c_string(s) if s is not None else None
            return None
        name, size = H.BASIC.get(t, ('', 0))
        if t == 0x40: return float_lit(struct.unpack('<f', raw[:4])[0], 'f')
        if t in (0x41, 0x42): return float_lit(struct.unpack('<d', raw[:8])[0], '')
        if t == 0x30: return 'true' if raw[0] else 'false'
        if not size: return None
        v = struct.unpack(INT_FMT[(size, not name.startswith('unsigned') and name != 'unsigned char')], raw[:size])[0]
        return str(v) + ('ull' if size == 8 and name.startswith('unsigned') else 'll' if size == 8 else 'u' if name.startswith('unsigned') else '')
    k, d = db.r.get(ti, (0, b''))
    if k == H.LF_MODIFIER: return literal(db, u32(d, 0), raw, pe, depth + 1)
    if k == H.LF_POINTER:
        v = u32(raw, 0)
        if v == 0: return 'nullptr'
        t = u32(d, 0)
        while db.kind(t) == H.LF_MODIFIER: t = u32(db.r[t][1], 0)
        if t in (0x70, 0x10, 0x20):
            s = pe.cstring(v)
            return c_string(s) if s is not None else None
        return None
    if k == H.LF_ENUM:
        info = db.udt(ti); v = literal(db, info['utype'], raw, pe, depth + 1)
        return '(%s)%s' % (db.spell(info['name']), v) if v is not None else None
    if k == H.LF_ARRAY:
        et = u32(d, 0); es = db.size(et); n = H.numeric(d, 8)[0] // es if es else 0
        items = [literal(db, et, raw[i * es:(i + 1) * es], pe, depth + 1) for i in range(n)]
        return '{' + ', '.join(items) + '}' if n and None not in items else None
    if k in H.CLASSLIKE:
        info = db.udt(db.resolve(ti)); s = H.simplify(info['name'])
        if s == 'sf::Color': return 'sf::Color(%d, %d, %d, %d)' % tuple(raw[:4])
        m = re.fullmatch(r'sf::Vector2<(float|int|unsigned int)>', s)
        if m:
            e = 0x40 if m.group(1) == 'float' else 0x74 if m.group(1) == 'int' else 0x75
            return '%s(%s, %s)' % (s, literal(db, e, raw[:4], pe), literal(db, e, raw[4:8], pe))
        F = db.fields(info['field'])
        if info['kind'] not in H.CLASSLIKE or any(f[0] in ('base', 'vbase', 'vftable') or
                                                   (f[0] == 'method' and f[4] == H.split_q(info['name'])[-1]) for f in F):
            return None
        items = [literal(db, f[2], raw[f[3]:f[3] + db.size(f[2])], pe, depth + 1)
                 for f in sorted((f for f in F if f[0] == 'member'), key=lambda f: f[3])]
        return '{' + ', '.join(items) + '}' if items and None not in items else None
    return None

def global_data(streams):
    """Name -> (type, offset, segment) for S_GDATA32/S_LDATA32 in the global symbol stream."""
    dbi = streams[3]; sy = streams[u16(dbi, 20)]; p, out = 0, {}
    while p + 4 <= len(sy):
        ln, k = u16(sy, p), u16(sy, p + 2)
        if ln < 2: break
        if k in (0x110c, 0x110d): out.setdefault(cstr(sy, p + 14)[0], (u32(sy, p + 4), u32(sy, p + 8), u16(sy, p + 12)))
        p += 2 + ln
    return out

# ---------------------------------------------------------------- stubs

def named_args(db, arglist):
    k, d = db.r.get(arglist, (0, b''))
    if k != H.LF_ARGLIST: return [], ''
    types, parts = [], []
    for i in range(u32(d, 0)):
        t = u32(d, 4 + 4 * i)
        if t == 0: parts.append('...')
        elif t != 0x03: types.append(t); parts.append(db.decl(t, 'a%d' % i))
    return types, ', '.join(parts)

def ctor_args(gen, db, ti):
    """Arguments that construct a game class with no default constructor, or None if it has one."""
    info = db.udt(db.resolve(ti))
    if not info or info['name'] not in gen.game: return None
    ctors = [m for m in gen.methods(gen.game[info['name']]) if m['ctor']]
    if not ctors: return None
    sized = [(len(named_args(db, m['arglist'])[0]), m) for m in ctors]
    if any(n == 0 for n, _ in sized): return None
    n, m = min(sized, key=lambda x: x[0])
    return ', '.join('recon::stub_value<%s>()' % db.decl(t) for t in named_args(db, m['arglist'])[0])

def init_list(gen, db, ti):
    info = db.udt(ti); inits = []
    if info['kind'] == H.LF_UNION: return ''
    for k, a, x, o, n in db.fields(info['field']):
        if k == 'base':
            args = ctor_args(gen, db, x)
            if args is not None: inits.append('%s(%s)' % (db.decl(x), args))
        elif k == 'member':
            kind = db.kind(x)
            if kind == H.LF_POINTER and ((u32(db.r[x][1], 4) >> 5) & 7) in (1, 4):
                inits.append('%s(recon::stub_value<%s>())' % (n, db.decl(x)))
            elif kind == H.LF_MODIFIER and u16(db.r[x][1], 4) & 1 and db.kind(u32(db.r[x][1], 0)) not in H.CLASSLIKE:
                inits.append('%s(recon::stub_value<%s>())' % (n, db.decl(u32(db.r[x][1], 0))))
            elif kind in H.CLASSLIKE:
                args = ctor_args(gen, db, x)
                if args is not None: inits.append('%s(%s)' % (n, args)); continue
                # Ordered containers whose comparator has no default constructor need one passed in.
                s = H.simplify(db.udt(db.resolve(x))['name'])
                inst = H.instantiations(s)
                pos = {'std::priority_queue': 2, 'std::map': 2, 'std::multimap': 2, 'std::set': 1, 'std::multiset': 1}
                if inst and inst[0][0] in pos and len(inst[0][1]) > pos[inst[0][0]]:
                    cmp = inst[0][1][pos[inst[0][0]]]
                    if cmp in gen.game:
                        cargs = ctor_args(gen, db, gen.game[cmp])
                        if cargs is not None: inits.append('%s(%s(%s))' % (n, cmp, cargs))
    return (' : ' + ', '.join(inits)) if inits else ''

def write_if_changed(path, text):
    """Keep timestamps of unchanged files so incremental builds stay incremental."""
    path = pathlib.Path(path)
    if path.exists() and path.read_text(encoding='utf-8') == text: return False
    path.write_text(text, encoding='utf-8'); return True

def translated_names(src_dir):
    names = set()
    for p in pathlib.Path(src_dir).glob('*.cpp'):
        for m in re.finditer(r'^[^\s/#].*?\b((?:[A-Za-z_]\w*::)+~?[A-Za-z_]\w*|(?:[A-Za-z_]\w*::)*operator\W+)\s*[(={;]',
                             p.read_text(encoding='utf-8', errors='replace'), re.M):
            names.add(m.group(1))
        for m in re.finditer(r'^(?:int|auto)\s+(main)\s*\(', p.read_text(encoding='utf-8', errors='replace'), re.M):
            names.add('main')
    return names

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('pdb', type=pathlib.Path); ap.add_argument('exe', type=pathlib.Path)
    ap.add_argument('--inventory', type=pathlib.Path, default=pathlib.Path('_local/pdb'))
    ap.add_argument('--recon', type=pathlib.Path, default=pathlib.Path('recon'))
    a = ap.parse_args()
    streams, ipi, db, gen, compiled = H.load(a.pdb, a.inventory)
    pe = PE(a.exe); done = translated_names(a.recon / 'src')
    stubs = collections.defaultdict(list); counts = collections.Counter()

    def unit_of(qual, name, top):
        c = compiled.get(qual + '::' + name)
        return c['unit'] if c else default_unit.get(top, H.fname(top)[:-2])
    default_unit = {}
    for n, c in compiled.items():
        if '::' in n:
            top = gen.top_of(n.rsplit('::', 1)[0])[1]
            default_unit.setdefault(top, collections.Counter())[c['unit']] += 1
    default_unit = {t: c.most_common(1)[0][0] for t, c in default_unit.items()}

    for qual, ti in sorted(gen.game.items()):
        info = db.udt(ti)
        if info['kind'] == H.LF_ENUM: continue
        short = H.split_q(qual)[-1]; top = gen.top_of(qual)[1]
        for m in gen.methods(ti):
            full = qual + '::' + m['name']
            if m['pure'] or full in done: counts['translated' if full in done else 'pure'] += 1; continue
            types, params = named_args(db, m['arglist'])
            cv = ' const' if m['const'] else ''
            if m['ctor']: head = '%s::%s(%s)%s' % (qual, short, params, init_list(gen, db, ti)); ret = None
            elif m['dtor']: head = '%s::%s()' % (qual, m['name']); ret = None
            else:
                sig = '%s::%s(%s)%s' % (qual, m['name'], params, cv)
                head = sig if m['conv'] else db.decl(m['rv'], sig)
                ret = db.decl(m['rv']) if m['rv'] != 0x03 else None
            body = ' RECON_UNIMPLEMENTED("%s");' % full + (' return recon::stub_value<%s>();' % ret if ret else '')
            stubs[unit_of(qual, m['name'], top)].append('%s {%s }' % (head, body)); counts['stub'] += 1

    # Non-member game functions: declarations + stubs (deduplicated by signature).
    free_decl = collections.defaultdict(list); seen = set()
    for name, c in sorted(compiled.items()):
        if c['unit'] in TOOL_UNITS or NOT_GAME_FUNCTIONS.match(name): continue
        owner = name.rsplit('::', 1)[0] if '::' in name else ''
        if (owner and (owner in db.full or not H.gen_ok(owner))) or not H.gen_ok(name) or name == 'main': continue
        if not re.fullmatch(r'(?:[A-Za-z_]\w*::)*(?:[A-Za-z_]\w*|operator\W+)', name) or db.kind(c['type']) != H.LF_PROCEDURE: continue
        d = db.r[c['type']][1]; types, params = named_args(db, u32(d, 8))
        segs = H.split_q(name); ns, short = segs[:-1], segs[-1]
        proto = db.decl(u32(d, 0), '%s(%s)' % (short, db.args(u32(d, 8))))
        if (name, proto) in seen: continue
        seen.add((name, proto))
        free_decl[tuple(ns)].append(proto + ';')
        if name in done: counts['translated'] += 1; continue
        rv = u32(d, 0); ret = db.decl(rv) if rv != 0x03 else None
        head = db.decl(rv, '%s(%s)' % (name, params))
        stubs[c['unit']].append('%s {%s }' % (head, ' RECON_UNIMPLEMENTED("%s");' % name +
                                              (' return recon::stub_value<%s>();' % ret if ret else '')))
        counts['free_stub'] += 1
    inc = a.recon / 'include'
    lines = ['// Generated by tools/pdb_stubs.py: non-member game functions.', '#pragma once', '#include "all.h"']
    for ns in sorted(free_decl):
        if ns: lines.append(' '.join('namespace %s {' % n for n in ns))
        lines += free_decl[ns]
        if ns: lines.append('}' * len(ns))
    write_if_changed(inc / 'free_functions.h', '\n'.join(lines) + '\n')
    write_if_changed(inc / 'recon_all.h', '#pragma once\n#include "all.h"\n#include "free_functions.h"\n#include "recon_stub.h"\n')

    out = a.recon / 'stubs'; out.mkdir(exist_ok=True)
    wanted = {}
    for unit, defs in sorted(stubs.items()):
        wanted['%s.stubs.cpp' % re.sub(r'\W+', '_', unit)] = (
            '// Generated by tools/pdb_stubs.py: functions of %s.cpp not yet translated.\n#include "recon_all.h"\n\n' % unit
            + '\n'.join(defs) + '\n')
    for old in out.glob('*.stubs.cpp'):
        if old.name not in wanted: old.unlink()
    for name, text in wanted.items(): write_if_changed(out / name, text)

    # Static data members with their image values.
    gdata = global_data(streams); lines = ['// Generated by tools/pdb_stubs.py: static data members, values from vagante.exe.',
                                           '#include "recon_all.h"', '#include <limits>', '']
    for qual, ti in sorted(gen.game.items()):
        info = db.udt(ti)
        if info['kind'] == H.LF_ENUM: continue
        for k, at, x, o, n in db.fields(info['field']):
            if k != 'static': continue
            full = qual + '::' + n
            if full in done: continue
            sym = gdata.get(full); init, note = None, 'not in the image (optimized out or defined elsewhere)'
            if sym:
                s = pe.sec[sym[2] - 1] if 0 < sym[2] <= len(pe.sec) else None
                raw = pe.read(s['rva'] + sym[1], db.size(x) or 4)[0] if s else None
                init = literal(db, x, raw, pe); note = None
                if raw is not None and not any(raw) and s and s['flags'] & 0x80000000:
                    note = 'zero in a writable section: may be set by a dynamic initializer'
                elif init is None:
                    note = 'no constant initializer (class type or dynamic initializer)'
            counts['static_value' if init is not None else 'static_default'] += 1
            if init is None and db.kind(x) == H.LF_MODIFIER and u16(db.r[x][1], 4) & 1:
                init = '{}'; note = 'TODO value: ' + (note or 'unknown')   # a const needs an initializer
            lines.append('%s%s;%s' % (db.decl(x, full), (' = ' + init) if init is not None else '',
                                       ('  // ' + note) if note else ''))
    write_if_changed(out / 'statics.cpp', '\n'.join(lines) + '\n')
    print(dict(counts), 'units:', len(stubs))

if __name__ == '__main__': main()
