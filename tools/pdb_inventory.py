"""Read-only PDB inventory: modules, source files, functions and C++ types.

Parses MSF 7 / DBI / TPI directly (no external packages). Writes derived
metadata to _local/pdb/; never copies code or data from the game.

    python tools/pdb_inventory.py F:/SteamLibrary/steamapps/common/vagante/vagante.pdb
"""
import argparse, collections, json, pathlib, re, struct

def u8(b, p): return b[p]
def u16(b, p): return struct.unpack_from('<H', b, p)[0]
def u32(b, p): return struct.unpack_from('<I', b, p)[0]
def i32(b, p): return struct.unpack_from('<i', b, p)[0]
def cstr(b, p):
    e = b.index(b'\0', p)
    return b[p:e].decode('utf-8', 'replace'), e + 1

# ---------------------------------------------------------------- MSF

def msf_streams(b):
    if not b.startswith(b'Microsoft C/C++ MSF 7.00'): raise ValueError('Not MSF 7')
    page, dir_size, blockmap = u32(b, 32), u32(b, 44), u32(b, 52)
    ndir = (dir_size + page - 1) // page
    dir_pages = [u32(b, blockmap * page + 4 * i) for i in range(ndir)]
    d = b''.join(b[p * page:(p + 1) * page] for p in dir_pages)[:dir_size]
    n = u32(d, 0); sizes = [u32(d, 4 + 4 * i) for i in range(n)]; at = 4 + 4 * n
    out = []
    for size in sizes:
        if size == 0xffffffff: out.append(b''); continue
        cnt = (size + page - 1) // page
        pages = struct.unpack_from('<%dI' % cnt, d, at); at += 4 * cnt
        out.append(b''.join(b[p * page:(p + 1) * page] for p in pages)[:size])
    return out

# ---------------------------------------------------------------- numeric leaves

NUM = {0x8000: ('<b', 1), 0x8001: ('<h', 2), 0x8002: ('<H', 2), 0x8003: ('<i', 4),
       0x8004: ('<I', 4), 0x8009: ('<q', 8), 0x800a: ('<Q', 8)}

def numeric(b, p):
    v = u16(b, p)
    if v < 0x8000: return v, p + 2
    fmt, n = NUM.get(v, (None, 0))
    if fmt is None: raise ValueError('numeric leaf 0x%x' % v)
    return struct.unpack_from(fmt, b, p + 2)[0], p + 2 + n

# ---------------------------------------------------------------- TPI

LF_CLASS, LF_STRUCTURE, LF_INTERFACE, LF_UNION, LF_ENUM = 0x1504, 0x1505, 0x1519, 0x1506, 0x1507
LF_FIELDLIST, LF_METHODLIST, LF_MFUNCTION = 0x1203, 0x1206, 0x1009
LF_POINTER, LF_MODIFIER, LF_ARRAY, LF_PROCEDURE, LF_BITFIELD = 0x1002, 0x1001, 0x1503, 0x1008, 0x1205
ACCESS = {0: '', 1: 'private', 2: 'protected', 3: 'public'}
BASIC = {0x03: 'void', 0x10: 'char', 0x20: 'uchar', 0x11: 'short', 0x21: 'ushort', 0x12: 'long',
         0x22: 'ulong', 0x13: 'int64', 0x23: 'uint64', 0x30: 'bool', 0x40: 'float', 0x41: 'double',
         0x68: 'int8', 0x69: 'uint8', 0x70: 'char', 0x71: 'wchar_t', 0x72: 'int16', 0x73: 'uint16',
         0x74: 'int', 0x75: 'uint', 0x76: 'int64', 0x77: 'uint64', 0x7a: 'char16_t', 0x7b: 'char32_t'}

def tpi_records(s):
    hdr, begin = u32(s, 4), u32(s, 8)
    recs, p, ti = {}, hdr, begin
    while p + 4 <= len(s):
        ln = u16(s, p)
        recs[ti] = (u16(s, p + 2), s[p + 4:p + 2 + ln]); p += 2 + ln; ti += 1
    return recs

def parse_fieldlist(recs, ti, seen=None):
    seen = seen or set()
    if ti in seen or ti not in recs: return []
    seen.add(ti)
    kind, d = recs[ti]
    if kind != LF_FIELDLIST: return []
    out, p = [], 0
    while p + 2 <= len(d):
        if d[p] >= 0xf0: p += d[p] & 0x0f; continue
        k = u16(d, p); p += 2
        try:
            if k == 0x1400:  # BCLASS
                attr, idx = u16(d, p), u32(d, p + 2); off, p = numeric(d, p + 6)
                out.append(dict(k='base', type=idx, offset=off, access=ACCESS[attr & 3]))
            elif k in (0x1401, 0x1402):  # VBCLASS / IVBCLASS
                attr, idx = u16(d, p), u32(d, p + 2); _, p = numeric(d, p + 10); _, p = numeric(d, p)
                out.append(dict(k='vbase', type=idx, access=ACCESS[attr & 3]))
            elif k == 0x150d:  # MEMBER
                attr, t = u16(d, p), u32(d, p + 2); off, p = numeric(d, p + 6); name, p = cstr(d, p)
                out.append(dict(k='member', type=t, offset=off, name=name, access=ACCESS[attr & 3]))
            elif k == 0x150e:  # STMEMBER
                attr, t = u16(d, p), u32(d, p + 2); name, p = cstr(d, p + 6)
                out.append(dict(k='static', type=t, name=name))
            elif k == 0x150f:  # METHOD (overload set)
                cnt, ml = u16(d, p), u32(d, p + 2); name, p = cstr(d, p + 6)
                out.append(dict(k='method', name=name, count=cnt, mlist=ml))
            elif k == 0x1511:  # ONEMETHOD
                attr, t = u16(d, p), u32(d, p + 2); p += 6
                mprop = (attr >> 2) & 7
                if mprop in (4, 6): p += 4
                name, p = cstr(d, p)
                out.append(dict(k='method', name=name, count=1, virtual=mprop in (1, 4, 5, 6), type=t))
            elif k == 0x1510:  # NESTTYPE
                idx = u32(d, p + 2); name, p = cstr(d, p + 6)
                out.append(dict(k='nested', type=idx, name=name))
            elif k == 0x1409:  # VFUNCTAB
                p += 6; out.append(dict(k='vftable'))
            elif k == 0x1502:  # ENUMERATE
                v, p = numeric(d, p + 2); name, p = cstr(d, p)
                out.append(dict(k='enumerator', name=name, value=v))
            elif k == 0x1404:  # INDEX (continuation)
                out.extend(parse_fieldlist(recs, u32(d, p + 2), seen)); p += 6
            else:
                break  # unknown subleaf: stop safely
        except (ValueError, IndexError, struct.error):
            break
    return out

def method_virtuals(recs, ml, count):
    """Count virtual entries in an LF_METHODLIST."""
    kind, d = recs.get(ml, (0, b''))
    if kind != LF_METHODLIST: return 0
    p, v = 0, 0
    for _ in range(count):
        if p + 8 > len(d): break
        mprop = (u16(d, p) >> 2) & 7; p += 8
        if mprop in (4, 6): p += 4
        if mprop in (1, 4, 5, 6): v += 1
    return v

def type_name(recs, ti, depth=0):
    if ti < 0x1000:
        base = BASIC.get(ti & 0xff, 'T%x' % ti)
        return base + ('*' if ti & 0x700 else '')
    if depth > 6 or ti not in recs: return 'T%x' % ti
    kind, d = recs[ti]
    try:
        if kind in (LF_CLASS, LF_STRUCTURE, LF_INTERFACE):
            _, p = numeric(d, 16); return cstr(d, p)[0]
        if kind == LF_UNION:
            _, p = numeric(d, 8); return cstr(d, p)[0]
        if kind == LF_ENUM: return cstr(d, 12)[0]
        if kind == LF_POINTER:
            mode = (u32(d, 4) >> 5) & 7
            return type_name(recs, u32(d, 0), depth + 1) + ('&' if mode in (1, 3) else '*')
        if kind == LF_MODIFIER: return type_name(recs, u32(d, 0), depth + 1)
        if kind == LF_ARRAY:
            size, _ = numeric(d, 8); return '%s[%d bytes]' % (type_name(recs, u32(d, 0), depth + 1), size)
        if kind == LF_BITFIELD: return '%s:%d' % (type_name(recs, u32(d, 0), depth + 1), d[4])
        if kind in (LF_PROCEDURE, LF_MFUNCTION): return 'fn'
    except (ValueError, IndexError, struct.error):
        pass
    return 'T%x' % ti

def types(recs):
    classes, enums = {}, {}
    for ti, (kind, d) in recs.items():
        try:
            if kind in (LF_CLASS, LF_STRUCTURE, LF_INTERFACE, LF_UNION):
                if kind == LF_UNION:
                    cnt, prop, fl = u16(d, 0), u16(d, 2), u32(d, 4); size, p = numeric(d, 8)
                else:
                    cnt, prop, fl = u16(d, 0), u16(d, 2), u32(d, 4); size, p = numeric(d, 16)
                if prop & 0x80: continue  # forward reference
                name, _ = cstr(d, p)
                if name in classes and classes[name]['size'] >= size: continue
                fields = parse_fieldlist(recs, fl)
                members = [dict(name=f['name'], offset=f['offset'], type=type_name(recs, f['type']))
                           for f in fields if f['k'] == 'member']
                methods = [f for f in fields if f['k'] == 'method']
                nvirt = sum(1 for m in methods if m.get('virtual')) + sum(
                    method_virtuals(recs, m['mlist'], m['count']) for m in methods if 'mlist' in m)
                classes[name] = dict(
                    kind={LF_CLASS: 'class', LF_STRUCTURE: 'struct', LF_INTERFACE: 'interface', LF_UNION: 'union'}[kind],
                    size=size, bases=[type_name(recs, f['type']) for f in fields if f['k'] in ('base', 'vbase')],
                    has_vftable=any(f['k'] == 'vftable' for f in fields) or nvirt > 0,
                    members=members, statics=[f['name'] for f in fields if f['k'] == 'static'],
                    methods=sorted({m['name'] for m in methods}), method_count=sum(m['count'] for m in methods),
                    virtual_methods=nvirt)
            elif kind == LF_ENUM:
                cnt, prop, fl = u16(d, 0), u16(d, 2), u32(d, 8)
                if prop & 0x80: continue
                name, _ = cstr(d, 12)
                enums[name] = [(f['name'], f['value']) for f in parse_fieldlist(recs, fl) if f['k'] == 'enumerator']
        except (ValueError, IndexError, struct.error):
            continue
    return classes, enums

# ---------------------------------------------------------------- DBI

def dbi_modules(dbi):
    mod_size, sc_size, smap_size, src_size = i32(dbi, 24), i32(dbi, 28), i32(dbi, 32), i32(dbi, 36)
    p, end, mods = 64, 64 + mod_size, []
    while p + 64 <= end:
        sym_stream, sym_bytes = u16(dbi, p + 34), u32(dbi, p + 36)
        name, q = cstr(dbi, p + 64); obj, q = cstr(dbi, q)
        mods.append(dict(name=name, obj=obj, stream=sym_stream, sym_bytes=sym_bytes, sources=[]))
        p = (q + 3) & ~3
    # source-info substream: module -> source files
    s = 64 + mod_size + sc_size + smap_size
    nmod = u16(dbi, s); idx = s + 4; cnts = idx + 2 * nmod; offs = cnts + 2 * nmod
    counts = [u16(dbi, cnts + 2 * i) for i in range(nmod)]
    names = offs + 4 * sum(counts); k = 0
    for i in range(min(nmod, len(mods))):
        for _ in range(counts[i]):
            mods[i]['sources'].append(cstr(dbi, names + u32(dbi, offs + 4 * k))[0]); k += 1
    return mods

PROC = {0x1110, 0x110f, 0x1147, 0x1146}

def module_functions(stream, sym_bytes):
    out, p = [], 4
    while p + 4 <= min(sym_bytes, len(stream)):
        ln, kind = u16(stream, p), u16(stream, p + 2)
        if ln < 2: break
        if kind in PROC:
            r = p + 4
            out.append(dict(name=cstr(stream, r + 35)[0], size=u32(stream, r + 12),
                            rva_offset=u32(stream, r + 28), section=u16(stream, r + 32)))
        p += 2 + ln
    return out

# ---------------------------------------------------------------- classification

LIB_PATTERNS = [
    ('msvc-crt/stl', re.compile(r'(?i)(microsoft visual studio|\\vc\\tools|\\crt\\|vctools|f:\\dd\\|d:\\agent\\|\\ucrt\\|windows kits|\* (linker|cil))')),
    ('sfml', re.compile(r'(?i)sfml')),
    ('steam', re.compile(r'(?i)steam')),
    ('boost', re.compile(r'(?i)boost')),
    ('box2d/physics', re.compile(r'(?i)box2d')),
    ('json', re.compile(r'(?i)(json|rapidjson)')),
    ('zlib', re.compile(r'(?i)zlib')),
    ('openal/audio', re.compile(r'(?i)(openal|vorbis|ogg|flac)')),
    ('image', re.compile(r'(?i)(stb_|libpng|jpeg|freetype)')),
]

GAME_OBJ = re.compile(r'(?i)projectrogue\\msvc\\vagante\\')
LIB_FN = re.compile(r'^(`|std::|boost::|sf::|rapidjson::|Concurrency::|__|_|operator new|operator delete|stdext::|Json::|nlohmann::|fmt::|\?)')

def classify(mod):
    if GAME_OBJ.search(mod['obj'] or mod['name']): return 'game'
    hay = mod['obj'] or mod['name']
    if 'libraries\\curl' in hay.lower(): return 'curl'
    for label, rx in LIB_PATTERNS:
        if rx.search(hay): return label
    return 'other-lib'

def is_game_fn(name):
    return not LIB_FN.search(name)

def main():
    a = argparse.ArgumentParser()
    a.add_argument('pdb', type=pathlib.Path)
    a.add_argument('--out', type=pathlib.Path, default=pathlib.Path('_local/pdb'))
    args = a.parse_args(); args.out.mkdir(parents=True, exist_ok=True)
    streams = msf_streams(args.pdb.read_bytes())
    recs = tpi_records(streams[2])
    classes, enums = types(recs)
    mods = dbi_modules(streams[3])
    for m in mods:
        m['category'] = classify(m)
        st = m['stream']
        m['functions'] = module_functions(streams[st], m['sym_bytes']) if st != 0xffff and st < len(streams) else []
    game = [m for m in mods if m['category'] == 'game']
    (args.out / 'modules.json').write_text(json.dumps(mods, indent=1))
    (args.out / 'classes.json').write_text(json.dumps(classes, indent=1, sort_keys=True))
    (args.out / 'enums.json').write_text(json.dumps(enums, indent=1, sort_keys=True))

    cat = collections.defaultdict(lambda: [0, 0, 0])
    for m in mods:
        c = cat[m['category']]; c[0] += 1; c[1] += len(m['functions']); c[2] += sum(f['size'] for f in m['functions'])
    # Game-owned functions, deduplicated by name (COMDAT folding repeats inline members per module).
    owned = {}
    for m in game:
        unit = pathlib.PureWindowsPath(m['obj'] or m['name']).stem
        for f in m['functions']:
            if is_game_fn(f['name']) and f['name'] not in owned:
                owned[f['name']] = dict(unit=unit, size=f['size'])
    per_unit = collections.defaultdict(lambda: [0, 0])
    for f in owned.values():
        per_unit[f['unit']][0] += 1; per_unit[f['unit']][1] += f['size']
    per_class = collections.Counter(n.rsplit('::', 1)[0] for n in owned if '::' in n)
    game_classes = {n: c for n, c in classes.items() if n in per_class}
    summary = dict(type_records=len(recs), classes_total=len(classes), enums=len(enums), modules=len(mods),
                   by_category={k: dict(modules=v[0], functions=v[1], code_bytes=v[2]) for k, v in sorted(cat.items())},
                   game_units=len(game), game_functions=len(owned), game_code_bytes=sum(f['size'] for f in owned.values()),
                   game_classes_with_code=len(game_classes),
                   units=sorted(({'unit': u, 'functions': v[0], 'code_bytes': v[1]} for u, v in per_unit.items()),
                                key=lambda x: -x['code_bytes']))
    (args.out / 'summary.json').write_text(json.dumps(summary, indent=1))
    (args.out / 'game_functions.json').write_text(json.dumps(owned, indent=1, sort_keys=True))
    (args.out / 'game_classes.json').write_text(json.dumps(game_classes, indent=1, sort_keys=True))
    print(json.dumps({k: v for k, v in summary.items() if k != 'units'}, indent=1))
    print('game units (top 40 by code size):')
    for g in summary['units'][:40]:
        print('  %7d B %5d fn  %s' % (g['code_bytes'], g['functions'], g['unit']))

if __name__ == '__main__': main()
