"""Generate C++ headers for Vagante's own types from vagante.pdb.

Emits one header per top-level game type (layout, bases, nested types,
methods and vtable slots) plus a layout check that clang verifies against
the PDB sizes/offsets in the MSVC x86 ABI.

    python tools/pdb_headers.py F:/.../vagante.pdb --out recon/include
    clang++ --target=i686-pc-windows-msvc -std=c++17 -fsyntax-only \
        -Irecon/include recon/layout_check.cpp

Library types (std, boost, sf, ...) are declared in compat/lib_types.h. With
RECON_LAYOUT_CHECK they are size/alignment-exact opaque blobs; the port build
will provide the real types instead.
"""
import argparse, collections, hashlib, json, pathlib, re, sys
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from pdb_inventory import msf_streams, tpi_records, numeric, cstr, u16, u32, dbi_modules, module_functions, classify

LF_MODIFIER, LF_POINTER, LF_PROCEDURE, LF_MFUNCTION = 0x1001, 0x1002, 0x1008, 0x1009
LF_ARGLIST, LF_FIELDLIST, LF_BITFIELD, LF_METHODLIST = 0x1201, 0x1203, 0x1205, 0x1206
LF_ARRAY, LF_CLASS, LF_STRUCTURE, LF_UNION, LF_ENUM, LF_INTERFACE = 0x1503, 0x1504, 0x1505, 0x1506, 0x1507, 0x1519
LF_CLASS2, LF_STRUCTURE2 = 0x1608, 0x1609
CLASSLIKE = {LF_CLASS, LF_STRUCTURE, LF_UNION, LF_INTERFACE, LF_CLASS2, LF_STRUCTURE2}
KEY = {LF_CLASS: 'class', LF_STRUCTURE: 'struct', LF_INTERFACE: 'struct', LF_UNION: 'union',
       LF_CLASS2: 'class', LF_STRUCTURE2: 'struct'}
LF_FUNC_ID, LF_MFUNC_ID = 0x1601, 0x1602
ACC = {0: '', 1: 'private', 2: 'protected', 3: 'public'}
BASIC = {0x00: ('void', 0), 0x03: ('void', 0), 0x08: ('long', 4), 0x10: ('signed char', 1), 0x20: ('unsigned char', 1),
         0x68: ('signed char', 1), 0x69: ('unsigned char', 1), 0x70: ('char', 1), 0x71: ('wchar_t', 2),
         0x7a: ('char16_t', 2), 0x7b: ('char32_t', 4), 0x11: ('short', 2), 0x21: ('unsigned short', 2),
         0x72: ('short', 2), 0x73: ('unsigned short', 2), 0x12: ('long', 4), 0x22: ('unsigned long', 4),
         0x74: ('int', 4), 0x75: ('unsigned int', 4), 0x13: ('long long', 8), 0x23: ('unsigned long long', 8),
         0x76: ('long long', 8), 0x77: ('unsigned long long', 8), 0x30: ('bool', 1), 0x31: ('short', 2),
         0x32: ('long', 4), 0x33: ('long long', 8), 0x40: ('float', 4), 0x41: ('double', 8), 0x42: ('long double', 8)}
LIB_RX = re.compile(r'^(std::|stdext::|boost::|sf::|rapidjson::|Concurrency::|ImGui|Im[A-Z]|ISteam|CSteamID|CGameID|'
                    r'CCallback|CCallResult|Steam\w*_t$|curl|Curl|CURL|_|tag[A-Z]|HWND|GUID|IUnknown|glm::|b2[A-Z]|'
                    r'moodycamel::|nlohmann::|fmt::|Json::|pugi::|tinyxml|TiXml|lua_|z_stream|gz_)')
KEYWORDS = {'const', 'volatile', 'unsigned', 'signed', 'int', 'char', 'short', 'long', 'float', 'double', 'bool',
            'void', 'wchar_t', 'char16_t', 'char32_t', '__int64', '__cdecl', '__stdcall', '__thiscall', '__fastcall',
            '__vectorcall', '__ptr64', '__ptr32', 'struct', 'class', 'enum', 'union', 'true', 'false'}
DROP = ('std::allocator<', 'std::less<', 'std::char_traits<', 'std::hash<', 'std::equal_to<', 'std::default_delete<')
STD_STRING = {'std::string': 'basic_string<char>', 'std::wstring': 'basic_string<wchar_t>'}

# ---------------------------------------------------------------- name handling

def match_close(s, j):
    depth = 0
    for k in range(j, len(s)):
        if s[k] == '<': depth += 1
        elif s[k] == '>':
            depth -= 1
            if depth == 0: return k
    return None

def split_top(s, sep):
    out, depth, cur, i = [], 0, '', 0
    while i < len(s):
        c = s[i]
        if c in '<(': depth += 1
        elif c in '>)': depth -= 1
        if depth == 0 and s.startswith(sep, i):
            out.append(cur); cur = ''; i += len(sep); continue
        cur += c; i += 1
    out.append(cur)
    return out

def split_q(name): return split_top(name, '::')

def simplify(s):
    s = s.strip()
    i = s.find('<')
    if i < 0 or s.startswith('<'): return s
    j = match_close(s, i)
    if j is None: return s
    head, rest = s[:i], s[j + 1:]
    r = simplify(rest)
    tail = (' ' + r) if rest[:1].isspace() and r else r
    args = [simplify(a) for a in split_top(s[i + 1:j], ',')]
    if head.startswith('std::'):
        while args and args[-1].startswith(DROP): args.pop()
        if head == 'std::basic_string' and args in (['char'], ['wchar_t']):
            return ('std::string' if args == ['char'] else 'std::wstring') + tail
        it = msvc_iterator(head, args)
        if it: return it + tail
    return head + '<' + ', '.join(args) + '>' + tail

def peel(s, *heads):
    """Strip nested single-argument wrappers: peel('a<b<X>>', 'a', 'b') -> 'X'."""
    for h in heads:
        if not (s.startswith(h + '<') and s.endswith('>')): return None
        s = s[len(h) + 1:-1].strip()
    return s

def msvc_iterator(head, args):
    """MSVC STL iterator internals -> standard container::iterator spelling."""
    kind = {'std::_Tree_iterator': 'iterator', 'std::_Tree_const_iterator': 'const_iterator',
            'std::_Vector_iterator': 'iterator', 'std::_Vector_const_iterator': 'const_iterator',
            'std::_List_iterator': 'iterator', 'std::_List_const_iterator': 'const_iterator'}.get(head)
    if not kind or len(args) != 1: return None
    if 'Tree' in head:
        x = peel(args[0], 'std::_Tree_val', 'std::_Tree_simple_types')
        if x is None: return None
        kv = peel(x, 'std::pair')
        if kv:
            k, v = [a.strip() for a in split_top(kv, ',')]
            if k.endswith(' const'): return 'std::map<%s, %s>::%s' % (k[:-6], v, kind)
        return 'std::set<%s>::%s' % (x, kind)
    if 'Vector' in head:
        x = peel(args[0], 'std::_Vector_val', 'std::_Simple_types')
        return 'std::vector<%s>::%s' % (x, kind) if x else None
    x = peel(args[0], 'std::_List_val', 'std::_List_simple_types')
    return 'std::list<%s>::%s' % (x, kind) if x else None

def instantiations(s):
    res, i = [], 0
    while True:
        j = s.find('<', i)
        if j < 0: return res
        m = re.search(r'((?:[A-Za-z_]\w*::)*[A-Za-z_]\w*)$', s[:j]); k = match_close(s, j)
        if m and k is not None:
            res.append((m.group(1), [a.strip() for a in split_top(s[j + 1:k], ',')]))
        i = j + 1

def tokens(s):
    for m in re.finditer(r'(?<![\w:])((?:[A-Za-z_]\w*::)*[A-Za-z_]\w*)', s):
        if m.group(1) not in KEYWORDS:
            yield m.group(1), s[m.end():m.end() + 1] == '<'

def ident(s):
    x = re.sub(r'\W+', '_', s).strip('_')
    return 'lib_' + (x if len(x) <= 72 else x[:60] + '_' + hashlib.md5(s.encode()).hexdigest()[:8])

def fname(top): return re.sub(r'\W+', '_', top) + '.h'

def port_spellable(s): return not any(x in s for x in ('<lambda', 'unnamed', '`', 'anonymous', '<unknown', '::<'))

HANDLE_HEADS = ('std::shared_ptr<', 'std::weak_ptr<', 'std::unique_ptr<', 'std::enable_shared_from_this<',
                'std::function<', 'std::initializer_list<')

def complete_args(s):
    """Game names inside a library type that must be complete (by-value template arguments)."""
    out = set()
    for m in re.finditer(r'(?<![\w:])((?:[A-Za-z_]\w*::)*[A-Za-z_]\w*)', s):
        after = s[m.end():].lstrip()
        if after.startswith(('*', '&', '<', '(')): continue
        depth, k = 0, m.start() - 1                       # find the template that directly encloses the name
        while k >= 0:
            if s[k] == '>': depth += 1
            elif s[k] == '<':
                if depth == 0: break
                depth -= 1
            k -= 1
        head = re.search(r'((?:[A-Za-z_]\w*::)*[A-Za-z_]\w*)$', s[:k]) if k > 0 else None
        if head and (head.group(1) + '<') in HANDLE_HEADS: continue
        out.add(m.group(1))
    return out

# ---------------------------------------------------------------- type database

class TypeDB:
    def __init__(self, recs, evidence=None):
        """evidence: class name -> set of LF_MFUNCTION indices used by that class's compiled code."""
        self.r, self._udt, self._fields, self._align, self.warn = recs, {}, {}, {}, collections.Counter()
        defs = collections.defaultdict(list)
        for ti in recs:
            info = self.udt(ti)
            if info and not info['prop'] & 0x80: defs[info['name']].append(ti)
        # Stale objects leave several definitions under one name; pick the one the linked code uses.
        self.full, self.choices = {}, {}
        evidence = evidence or {}
        for name, tis in defs.items():
            if len(tis) == 1: self.full[name] = tis[0]; continue
            used = evidence.get(name, set())
            def score(t):
                ms = {f[2] for f in self.fields(self.udt(t)['field']) if f[0] == 'method'}
                return (len(ms & used), t)
            best = max(tis, key=score)
            self.full[name] = best
            self.choices[name] = dict(chosen=best, candidates=tis, evidence=score(best)[0] > 0)
        self.simp = {}
        for name, ti in self.full.items(): self.simp.setdefault(simplify(name), ti)
        self.class_names = {simplify(n) for n, t in self.full.items() if self.udt(t)['kind'] in CLASSLIKE}
        self.aliases = {}

    def kind(self, ti): return self.r.get(ti, (0,))[0] if ti >= 0x1000 else 0

    def udt(self, ti):
        if ti in self._udt: return self._udt[ti]
        k, d = self.r.get(ti, (0, b'')); info = None
        try:
            if k in (LF_CLASS2, LF_STRUCTURE2):
                size, p = numeric(d, 18); info = dict(kind=k, prop=u32(d, 0), field=u32(d, 4), size=size, name=cstr(d, p)[0])
            elif k in (LF_CLASS, LF_STRUCTURE, LF_INTERFACE):
                size, p = numeric(d, 16); info = dict(kind=k, prop=u16(d, 2), field=u32(d, 4), size=size, name=cstr(d, p)[0])
            elif k == LF_UNION:
                size, p = numeric(d, 8); info = dict(kind=k, prop=u16(d, 2), field=u32(d, 4), size=size, name=cstr(d, p)[0])
            elif k == LF_ENUM:
                info = dict(kind=k, prop=u16(d, 2), field=u32(d, 8), utype=u32(d, 4), size=None, name=cstr(d, 12)[0])
        except (ValueError, IndexError):
            info = None
        self._udt[ti] = info
        return info

    def resolve(self, ti):
        info = self.udt(ti)
        if info and info['prop'] & 0x80: return self.full.get(info['name'], ti)
        return ti

    def methodlist(self, ml, count):
        k, d = self.r.get(ml, (0, b''))
        if k != LF_METHODLIST: return []
        out, p = [], 0
        for _ in range(count):
            if p + 8 > len(d): break
            a, t = u16(d, p), u32(d, p + 4); p += 8; vo = None
            if ((a >> 2) & 7) in (4, 6): vo = u32(d, p); p += 4
            out.append((a, t, vo))
        return out

    def fields(self, ti):
        if ti in self._fields: return self._fields[ti]
        out, seen, stack = [], set(), [ti]
        while stack:
            t = stack.pop(0)
            if t in seen or t not in self.r: continue
            seen.add(t); k, d = self.r[t]
            if k != LF_FIELDLIST: continue
            p = 0
            while p + 2 <= len(d):
                if d[p] >= 0xf0: p += d[p] & 0x0f; continue
                k2 = u16(d, p); p += 2
                if k2 == 0x1400:
                    a, x = u16(d, p), u32(d, p + 2); off, p = numeric(d, p + 6); out.append(('base', a, x, off, None))
                elif k2 in (0x1401, 0x1402):
                    a, x = u16(d, p), u32(d, p + 2); _, p = numeric(d, p + 10); _, p = numeric(d, p)
                    out.append(('vbase', a, x, 0, None))
                elif k2 == 0x1404: stack.append(u32(d, p + 2)); p += 6
                elif k2 == 0x1409: p += 6; out.append(('vftable', 0, 0, 0, None))
                elif k2 == 0x140a: p += 10
                elif k2 == 0x140b: p += 6
                elif k2 == 0x150c: _, p = cstr(d, p + 6)
                elif k2 == 0x150d:
                    a, x = u16(d, p), u32(d, p + 2); off, p = numeric(d, p + 6); n, p = cstr(d, p)
                    out.append(('member', a, x, off, n))
                elif k2 == 0x150e:
                    a, x = u16(d, p), u32(d, p + 2); n, p = cstr(d, p + 6); out.append(('static', a, x, 0, n))
                elif k2 == 0x150f:
                    c, ml = u16(d, p), u32(d, p + 2); n, p = cstr(d, p + 6)
                    out.extend(('method', a, x, vo, n) for a, x, vo in self.methodlist(ml, c))
                elif k2 == 0x1511:
                    a, x = u16(d, p), u32(d, p + 2); p += 6; vo = None
                    if ((a >> 2) & 7) in (4, 6): vo = u32(d, p); p += 4
                    n, p = cstr(d, p); out.append(('method', a, x, vo, n))
                elif k2 in (0x1510, 0x1512):
                    x = u32(d, p + 2); n, p = cstr(d, p + 6); out.append(('nested', 0, x, 0, n))
                elif k2 == 0x1502:
                    a = u16(d, p); v, p = numeric(d, p + 2); n, p = cstr(d, p); out.append(('enumerator', a, 0, v, n))
                else:
                    self.warn['unknown subleaf 0x%x' % k2] += 1; break
        self._fields[ti] = out
        return out

    def size(self, ti):
        if ti < 0x1000:
            mode = (ti >> 8) & 7
            return 4 if mode in (4, 5) else 8 if mode == 6 else BASIC.get(ti & 0xff, ('', 0))[1]
        k, d = self.r.get(ti, (0, b''))
        if k in (LF_MODIFIER, LF_BITFIELD): return self.size(u32(d, 0))
        if k == LF_POINTER: return (u32(d, 4) >> 13) & 0x3f
        if k == LF_ARRAY: return numeric(d, 8)[0]
        if k in CLASSLIKE: return self.udt(self.resolve(ti))['size']
        if k == LF_ENUM: return self.size(self.udt(ti)['utype'])
        return 0

    def polymorphic(self, ti, depth=0):
        info = self.udt(self.resolve(ti))
        if not info or info['kind'] not in CLASSLIKE or depth > 30: return False
        F = self.fields(info['field'])
        if any(f[0] == 'vftable' or (f[0] == 'method' and ((f[1] >> 2) & 7) in (1, 4, 5, 6)) for f in F): return True
        return any(self.polymorphic(f[2], depth + 1) for f in F if f[0] in ('base', 'vbase'))

    def align(self, ti):
        if ti < 0x1000:
            return 4 if (ti >> 8) & 7 else max(1, min(8, BASIC.get(ti & 0xff, ('', 1))[1]))
        if ti in self._align: return self._align[ti]
        self._align[ti] = 4
        k, d = self.r.get(ti, (0, b''))
        if k in (LF_MODIFIER, LF_BITFIELD, LF_ARRAY): a = self.align(u32(d, 0))
        elif k == LF_POINTER: a = 4
        elif k == LF_ENUM: a = self.align(self.udt(ti)['utype'])
        elif k in CLASSLIKE:
            rt = self.resolve(ti); info = self.udt(rt); F = self.fields(info['field']) if info else []
            a = max([1] + [self.align(f[2]) for f in F if f[0] in ('member', 'base', 'vbase')] +
                    ([4] if self.polymorphic(ti) else []))
            p = self.pack(rt)
            if p: a = min(a, p)
        else: a = 4
        self._align[ti] = a
        return a

    def placements(self):
        """Record name -> offsets at which it is embedded by value in any defined record."""
        if hasattr(self, '_placed'): return self._placed
        self._placed = collections.defaultdict(list)
        for name, ti in self.full.items():
            info = self.udt(ti)
            if info['kind'] == LF_ENUM: continue
            for k, a, x, o, n in self.fields(info['field']):
                if k not in ('member', 'base'): continue
                while self.kind(x) in (LF_MODIFIER, LF_ARRAY): x = u32(self.r[x][1], 0)
                inner = self.udt(x)
                if inner: self._placed[inner['name']].append(o)
        return self._placed

    def pack(self, ti):
        """#pragma pack value for a packed record, inferred from its recorded offsets and size."""
        info = self.udt(ti)
        if not info or info['kind'] == LF_ENUM or not info['prop'] & 1: return None
        if 'pack' in info: return info['pack']
        info['pack'] = 1
        F = self.fields(info['field'])
        mems = sorted((f[3], self.size(f[2]), self.align(f[2])) for f in F
                      if f[0] == 'member' and self.kind(f[2]) != LF_BITFIELD)
        lowbit = lambda o: o & -o if o else 8
        if any(f[0] in ('base', 'vbase') for f in F) or self.polymorphic(ti):
            info['pack'] = max([1] + [min(a, lowbit(o)) for o, s, a in mems if o]); return info['pack']
        natural = max([1] + [a for o, s, a in mems])
        # Where the record is embedded elsewhere, its alignment must divide every such offset.
        placed = min([8] + [lowbit(o) for o in self.placements().get(info['name'], ()) if o])
        for n in (8, 4, 2, 1):
            if n > placed or (n >= natural and natural > 1 and n != 1): continue
            off, maxa, ok = 0, 1, True
            for o, s, a in mems:
                a = min(a, n); maxa = max(maxa, a)
                if info['kind'] == LF_UNION: off = max(off, s); continue
                if o < off: continue
                if (off + a - 1) // a * a != o: ok = False; break
                off = o + s
            if ok and (off + maxa - 1) // maxa * maxa == info['size']:
                info['pack'] = n; break
        return info['pack']

    # ------------------------------------------------------------ spelling

    def spell(self, name):
        s = simplify(name)
        if s not in self.aliases: return s
        # Spellable once the real library exists: layout mode uses the blob alias, port mode the real type.
        return 'RECON_EXT(%s, %s)' % (self.aliases[s], s) if port_spellable(s) else self.aliases[s]

    @staticmethod
    def _join(base, inner): return (base + ' ' + inner).rstrip() if inner else base

    def args(self, arglist):
        k, d = self.r.get(arglist, (0, b''))
        if k != LF_ARGLIST: return ''
        out = []
        for i in range(u32(d, 0)):
            t = u32(d, 4 + 4 * i)
            if t == 0: out.append('...')
            elif t != 0x03: out.append(self.decl(t))
        return ', '.join(out)

    def decl(self, ti, inner='', depth=0):
        if depth > 30: return self._join('int', inner)
        if ti < 0x1000:
            base = BASIC.get(ti & 0xff, ('int', 4))[0]
            return self._join(base, ('*' + inner) if (ti >> 8) & 7 else inner)
        k, d = self.r.get(ti, (0, b''))
        if k == LF_MODIFIER:
            a = u16(d, 4)
            return ('const ' if a & 1 else '') + ('volatile ' if a & 2 else '') + self.decl(u32(d, 0), inner, depth + 1)
        if k == LF_POINTER:
            t, a = u32(d, 0), u32(d, 4); mode = (a >> 5) & 7
            if mode in (2, 3): return 'unsigned char %s[%d]' % (inner or '', (a >> 13) & 0x3f)
            sym = {0: '*', 1: '&', 4: '&&'}.get(mode, '*')
            cv = (' const' if a & 0x400 else '') + (' volatile' if a & 0x200 else '')
            new = sym + cv + ((' ' + inner) if cv and inner else inner)
            if self.kind(t) in (LF_ARRAY, LF_PROCEDURE, LF_MFUNCTION): new = '(' + new + ')'
            return self.decl(t, new, depth + 1)
        if k == LF_ARRAY:
            et = u32(d, 0); size = numeric(d, 8)[0]; es = self.size(et)
            return self.decl(et, inner + '[%d]' % (size // es if es else 0), depth + 1)
        if k == LF_PROCEDURE: return self.decl(u32(d, 0), inner + '(' + self.args(u32(d, 8)) + ')', depth + 1)
        if k == LF_MFUNCTION: return self.decl(u32(d, 0), inner + '(' + self.args(u32(d, 16)) + ')', depth + 1)
        if k == LF_BITFIELD: return self.decl(u32(d, 0), inner, depth + 1)
        if k in CLASSLIKE or k == LF_ENUM: return self._join(self.spell(self.udt(ti)['name']), inner)
        return self._join('int', inner)

    def walk(self, ti, byval, out, depth=0):
        if ti < 0x1000 or depth > 30: return
        k, d = self.r.get(ti, (0, b''))
        if k in (LF_MODIFIER, LF_BITFIELD, LF_ARRAY): self.walk(u32(d, 0), byval, out, depth + 1)
        elif k == LF_POINTER: self.walk(u32(d, 0), False, out, depth + 1)
        elif k == LF_PROCEDURE: self.walk(u32(d, 0), False, out, depth + 1); self.walk(u32(d, 8), False, out, depth + 1)
        elif k == LF_MFUNCTION: self.walk(u32(d, 0), False, out, depth + 1); self.walk(u32(d, 16), False, out, depth + 1)
        elif k == LF_ARGLIST:
            for i in range(u32(d, 0)): self.walk(u32(d, 4 + 4 * i), False, out, depth + 1)
        elif k in CLASSLIKE or k == LF_ENUM: out.append((self.resolve(ti), byval))

    def deps(self, ti):
        info = self.udt(ti); out = []
        if not info or info['kind'] == LF_ENUM: return out
        for k, a, x, o, n in self.fields(info['field']):
            if k in ('base', 'vbase', 'member'): self.walk(x, True, out)
            elif k in ('static', 'method', 'nested'): self.walk(x, False, out)
        return out

# ---------------------------------------------------------------- generator

class Generator:
    def __init__(self, db, roots):
        self.db = db
        self.game = {}           # full name -> ti (game-owned types)
        self.ext = {}            # simplified external name -> {'ti', 'byval'}
        self.opaque = set()      # game names referenced but never defined
        q = [db.full[n] for n in sorted(roots) if n in db.full]
        while q:
            ti = q.pop(); name = db.udt(ti)['name']
            if name in self.game: continue
            self.game[name] = ti
            for dti, byval in db.deps(ti):
                dinfo = db.udt(dti)
                if not dinfo: continue
                dn = dinfo['name']
                if self.is_game_name(dn):
                    if dinfo['prop'] & 0x80 and dn not in db.full: self.opaque.add(dn)
                    elif dn not in self.game: q.append(db.full.get(dn, dti))
                else:
                    e = self.ext.setdefault(simplify(dn), dict(ti=dti, byval=False)); e['byval'] |= byval
                    for tok, _ in tokens(simplify(dn)):
                        if self.is_game_name(tok) and tok in db.full and tok not in self.game: q.append(db.full[tok])
        self.tops = {}           # top-level game name -> namespaces
        for name in self.game:
            ns, top = self.top_of(name)
            if top == name: self.tops[name] = ns
        self.plan_ext()

    def is_game_name(self, n):
        return not LIB_RX.search(n) and '<' not in n and '`' not in n and 'unnamed' not in n

    def top_of(self, name):
        segs = split_q(name)
        for i in range(1, len(segs) + 1):
            pref = '::'.join(segs[:i])
            if pref in self.db.full: return segs[:i - 1], pref
        return segs[:-1], name

    def is_nested_game(self, n): return n in self.game and self.top_of(n)[1] != n

    # ------------------------------------------------------------ external types

    def plan_ext(self):
        db = self.db
        heads = collections.defaultdict(set)
        for s in self.ext:
            for h, args in instantiations(s):
                heads[h].add(tuple('v' if re.fullmatch(r'-?\d+|true|false', a) else 't' for a in args))
        self.bad_heads = {h for h, ks in heads.items() if any('v' in k for k in ks) and len(ks) > 1}
        self.heads = {h: ks for h, ks in heads.items() if h not in self.bad_heads}
        for s, e in self.ext.items():
            if not self.spellable(s): db.aliases[s] = ident(s)
        self.plain = set()
        for s in self.ext:
            if s in db.aliases: continue
            for tok, is_tmpl in tokens(s):
                if not is_tmpl and tok not in STD_STRING and not self.is_game_name(tok):
                    self.plain.add(tok)

    def spellable(self, s):
        if any(x in s for x in ('<lambda', 'unnamed', '`', 'anonymous', '<unknown')): return False
        segs = split_q(s)
        for i in range(1, len(segs)):
            pref = '::'.join(segs[:i])
            if '<' in pref or pref in self.db.class_names: return False
        if '<' in s and '<' not in segs[-1]: return False
        if any(h in self.bad_heads for h, _ in instantiations(s)): return False
        for tok, is_tmpl in tokens(s):
            if is_tmpl or tok in STD_STRING: continue
            if self.is_game_name(tok):
                if self.is_nested_game(tok): return False
                continue
            tsegs = split_q(tok)
            if any('::'.join(tsegs[:i]) in self.db.class_names for i in range(1, len(tsegs))): return False
            ti = self.db.simp.get(tok)
            if ti is not None and self.db.udt(ti)['kind'] == LF_ENUM and tok != s: return False
        return True

    def blob(self, ti, label):
        db = self.db; info = db.udt(db.resolve(ti)); size = info['size'] or 0
        a = db.align(ti); F = db.fields(info['field'])
        if size <= 1 and not any(f[0] == 'member' for f in F): body = ''
        elif db.polymorphic(ti):
            body = ' virtual void _vf(); ' + ('unsigned char _[%d]; ' % (size - 4) if size > 4 else '')
        else: body = ' unsigned char _[%d]; ' % size
        return 'struct alignas(%d) %s {%s};' % (a, label, body)

    def lib_types_h(self):
        db = self.db
        ns_lines = collections.defaultdict(list)
        def put(qname, line_fn):
            segs = split_q(qname); ns_lines[tuple(segs[:-1])].append(line_fn(segs[-1]))
        ns_lines[('std',)] += ['template<class...> struct basic_string;', 'using string = basic_string<char>;',
                               'using wstring = basic_string<wchar_t>;']
        for h in sorted(self.heads):
            if h == 'std::basic_string': continue
            ks = next(iter(self.heads[h]))
            if any('v' in k for k in self.heads[h]):
                params = ', '.join(('auto A%d' if c == 'v' else 'class A%d') % i for i, c in enumerate(ks))
                put(h, lambda last, p=params: 'template<%s> struct %s;' % (p, last))
            else:
                put(h, lambda last: 'template<class...> struct %s;' % last)
        for tok in sorted(self.plain):
            ti = db.simp.get(tok)
            if ti is not None and db.udt(ti)['kind'] == LF_ENUM:
                put(tok, lambda last, t=ti: 'enum %s : %s;' % (last, db.decl(db.udt(t)['utype'])))
            elif tok not in db.class_names and ti is None and '::' not in tok:
                put(tok, lambda last: 'struct %s;' % last)
            else:
                put(tok, lambda last: 'struct %s;' % last)
        defs, alias_lines, opaque_lines = collections.defaultdict(list), [], []
        for s, e in sorted(self.ext.items()):
            ti = db.resolve(e['ti']); info = db.udt(ti)
            if s in db.aliases:
                if info['kind'] == LF_ENUM: line = 'using %s = %s;  // %s' % (db.aliases[s], db.decl(info['utype']), s)
                elif e['byval']: line = self.blob(ti, db.aliases[s]) + '  // ' + s
                else: line = 'struct %s;  // %s' % (db.aliases[s], s)
                (alias_lines if port_spellable(s) else opaque_lines).append(line)
                continue
            if info['kind'] == LF_ENUM or not e['byval']: continue
            if s in STD_STRING:
                defs[('std',)].append('template<> ' + self.blob(ti, STD_STRING[s]))
                continue
            segs = split_q(s); last = segs[-1]
            defs[tuple(segs[:-1])].append(('template<> ' if '<' in last else '') + self.blob(ti, last))
        out = ['// Generated by tools/pdb_headers.py. External (library) types used by the game.',
               '#pragma once',
               '#ifdef RECON_LAYOUT_CHECK',
               '#define RECON_EXT(alias, ...) alias',
               '#else',
               '#define RECON_EXT(alias, ...) __VA_ARGS__',
               '#include "port_lib_types.h"  // real libraries; must not depend on game headers',
               '#endif',
               '#include "../fwd.h"',
               '// Types with no spelling outside the PDB (lambdas, unnamed members): same-size placeholders.']
        out += opaque_lines
        out += ['#ifdef RECON_LAYOUT_CHECK',
                '// Layout mode: opaque blobs with the exact MSVC x86 size/alignment recorded in the PDB.']
        def emit(groups):
            for ns in sorted(groups):
                if ns: out.append(' '.join('namespace %s {' % n for n in ns))
                out.extend(groups[ns])
                if ns: out.append('}' * len(ns))
        emit(ns_lines); emit(defs); out.extend(alias_lines)
        out += ['#endif', '']
        return '\n'.join(out)

    # ------------------------------------------------------------ game types

    def emit_udt(self, ti, ind, lines, checks):
        db = self.db; info = db.udt(ti); name = info['name']; short = split_q(name)[-1]
        if info['kind'] == LF_ENUM:
            ut = db.decl(info['utype']); bits = 8 * (db.size(info['utype']) or 4)
            lines.append('%s%s %s : %s {' % (ind, 'enum class' if info['prop'] & 0x100 else 'enum', short, ut))
            signed = not ut.startswith('unsigned') and ut != 'bool'
            for k, a, x, v, n in db.fields(info['field']):
                if k != 'enumerator': continue
                if signed and v >= 1 << (bits - 1): v -= 1 << bits
                lines.append('%s    %s = %d,' % (ind, n, v))
            lines.append(ind + '};')
            return
        key = KEY[info['kind']]; F = db.fields(info['field'])
        head = '%s %s' % (key, short)
        bases = [('virtual ' if k == 'vbase' else '') + (ACC[a & 3] or 'public') + ' ' + db.decl(x)
                 for k, a, x, o, n in F if k in ('base', 'vbase')]
        if bases: head += ' : ' + ', '.join(bases)
        pack = db.pack(ti)
        if pack: lines.append('#pragma pack(push, %d)' % pack)
        lines.append('%s%s {  // size 0x%X' % (ind, head, info['size']))
        checks.append('static_assert(sizeof(%s) == %d, "size %s");' % (name, info['size'], name))
        cur = [None]  # always emit the first access label so layout checks can reach private members
        def acc(a):
            want = ACC[a & 3]
            if want and want != cur[0]: lines.append(ind + want + ':'); cur[0] = want
        sub = ind + '    '
        for k, a, x, o, n in F:                                    # nested types and typedefs
            if k != 'nested': continue
            nt = db.resolve(x); ni = db.udt(nt); fn = name + '::' + n
            if ni and ni['name'] == fn:
                if fn in self.game: acc(3); self.emit_udt(self.game[fn], sub, lines, checks)
            elif not (ni and ni['name'].startswith(name + '::<')):
                acc(3); lines.append('%susing %s = %s;' % (sub, n, db.decl(x)))
        mems = sorted(((o, i, a, x, n) for i, (k, a, x, o, n) in enumerate(F) if k == 'member'), key=lambda m: (m[0], m[1]))
        end, prev_bf, i = None, None, 0
        while i < len(mems):
            o, _, a, x, n = mems[i]; bf = db.kind(x) == LF_BITFIELD
            if bf:
                d = db.r[x][1]; store = db.size(u32(d, 0))
                if prev_bf == o: lines.append('%s%s : %d;' % (sub, db.decl(u32(d, 0), n), d[4])); i += 1; continue
                if end is not None and o < end:
                    lines.append('%s// overlaps (union in original): %s : %d;  // +0x%X' % (sub, db.decl(u32(d, 0), n), d[4], o)); i += 1; continue
                if end is not None and o > end: lines.append('%sunsigned char _pad_%X[%d];' % (sub, end, o - end))
                acc(a); lines.append('%s%s : %d;  // +0x%X' % (sub, db.decl(u32(d, 0), n), d[4], o))
                prev_bf, end = o, o + store; i += 1; continue
            prev_bf = None
            group = [mems[i]]; j = i + 1
            if info['kind'] == LF_UNION:                           # union members all live at +0
                acc(a); lines.append('%s%s;  // +0x%X' % (sub, db.decl(x, n), o))
                checks.append('static_assert(__builtin_offsetof(%s, %s) == %d, "%s::%s");' % (name, n, o, name, n))
                i += 1; continue
            while j < len(mems) and mems[j][0] == o and db.kind(mems[j][3]) != LF_BITFIELD: group.append(mems[j]); j += 1
            if end is not None and o < end:
                for g in group: lines.append('%s// overlaps (union in original): %s;  // +0x%X' % (sub, db.decl(g[3], g[4]), g[0]))
                i = j; continue
            al = max(db.align(g[3]) for g in group)
            if end is not None and o > (end + al - 1) // al * al:
                lines.append('%sunsigned char _pad_%X[%d];' % (sub, end, o - end))
            acc(a)
            if len(group) == 1:
                lines.append('%s%s;  // +0x%X' % (sub, db.decl(x, n), o))
            else:
                lines.append(sub + 'union {  // +0x%X' % o)
                lines.extend('%s    %s;' % (sub, db.decl(g[3], g[4])) for g in group)
                lines.append(sub + '};')
            for g in group:
                checks.append('static_assert(__builtin_offsetof(%s, %s) == %d, "%s::%s");' % (name, g[4], g[0], name, g[4]))
            end = o + max(db.size(g[3]) for g in group); i = j
        for k, a, x, o, n in F:                                    # static data
            if k == 'static': acc(a); lines.append('%sstatic %s;' % (sub, db.decl(x, n)))
        for m in self.methods(ti):                                 # methods
            pre = 'static ' if m['static'] else 'virtual ' if m['virtual'] else ''
            line = pre + m['decl'] + (' = 0' if m['pure'] else '') + ';'
            acc(m['attr'])
            lines.append(sub + line + ('  // vtbl +0x%X' % m['vo'] if m['vo'] is not None else ''))
        lines.append(ind + '};')
        if pack: lines.append('#pragma pack(pop)')

    def methods(self, ti):
        """Declared (non compiler-generated) methods of a record, deduplicated, in PDB order."""
        db = self.db; info = db.udt(ti); short = split_q(info['name'])[-1]; out, seen = [], set()
        for k, a, x, vo, n in db.fields(info['field']):
            if k != 'method' or a & 0x100 or n.startswith(('__', '`')) or db.kind(x) != LF_MFUNCTION: continue
            d = db.r[x][1]; rv, this, arglist = u32(d, 0), u32(d, 8), u32(d, 16); mprop = (a >> 2) & 7
            const = False
            if db.kind(this) == LF_POINTER:
                pt = u32(db.r[this][1], 0)
                const = db.kind(pt) == LF_MODIFIER and bool(u16(db.r[pt][1], 4) & 1)
            args = db.args(arglist)
            sig = '%s(%s)%s' % (n, args, ' const' if const else '')
            ctor, dtor = n == short, n.startswith('~')
            conv = n.startswith('operator ') and not re.match(r'operator (new|delete)\b', n)
            decl = sig if (ctor or dtor or conv) else db.decl(rv, sig)
            key = (decl, mprop == 2)
            if key in seen: continue
            seen.add(key)
            out.append(dict(name=n, attr=a, vo=vo, rv=rv, arglist=arglist, const=const, decl=decl,
                            ctor=ctor, dtor=dtor, conv=conv, static=mprop == 2,
                            virtual=mprop in (1, 4, 5, 6), pure=mprop in (5, 6)))
        return out

    def header(self, top, ns):
        db = self.db; inc = set()
        names = [n for n in self.game if n == top or n.startswith(top + '::')]
        for n in names:
            for dti, byval in db.deps(self.game[n]):
                dn = db.udt(dti)['name']
                if dn not in self.game:                        # library type: its game arguments
                    s = simplify(dn)
                    need = complete_args(s) if byval else set()
                    need |= {t for t, _ in tokens(s) if self.is_nested_game(t)}
                    inc.update(self.top_of(g)[1] for g in need if g in self.game and self.top_of(g)[1] != top)
                    continue
                t2 = self.top_of(dn)[1]
                if t2 != top and (byval or t2 != dn): inc.add(t2)
        lines = ['// Generated by tools/pdb_headers.py from vagante.pdb: reconstructed layout, not original source.',
                 '#pragma once', '#include "fwd.h"', '#include "compat/lib_types.h"']
        lines += ['#include "%s"' % fname(t) for t in sorted(inc)]
        lines.append('')
        if ns: lines.append(' '.join('namespace %s {' % n for n in ns))
        checks = []
        self.emit_udt(self.game[top], '', lines, checks)
        if ns: lines.append('}' * len(ns))
        lines.append('')
        return '\n'.join(lines), sorted(inc), checks

    def fwd_h(self):
        db = self.db; groups = collections.defaultdict(list)
        for top, ns in sorted(self.tops.items()):
            info = db.udt(self.game[top]); last = split_q(top)[-1]
            if info['kind'] == LF_ENUM:
                groups[tuple(ns)].append('%s %s : %s;' % ('enum class' if info['prop'] & 0x100 else 'enum', last, db.decl(info['utype'])))
            else:
                groups[tuple(ns)].append('%s %s;' % (KEY[info['kind']], last))
        for n in sorted(self.opaque):
            segs = split_q(n)
            if not any('::'.join(segs[:i]) in db.class_names for i in range(1, len(segs))):
                groups[tuple(segs[:-1])].append('struct %s;  // never defined in the PDB' % segs[-1])
        out = ['// Generated by tools/pdb_headers.py. Forward declarations of every game type.', '#pragma once']
        for ns in sorted(groups):
            if ns: out.append(' '.join('namespace %s {' % n for n in ns))
            out.extend(groups[ns])
            if ns: out.append('}' * len(ns))
        return '\n'.join(out) + '\n'

def write_if_changed(path, text):
    """Keep timestamps of unchanged files so incremental builds (and the PCH) stay valid."""
    path = pathlib.Path(path)
    if path.exists() and path.read_text(encoding='utf-8') == text: return False
    path.write_text(text, encoding='utf-8'); return True

def toposort(graph):
    order, state, cycles = [], {}, []
    def visit(n, stack):
        if state.get(n) == 2: return
        if state.get(n) == 1: cycles.append(stack[stack.index(n):] + [n]); return
        state[n] = 1
        for m in sorted(graph.get(n, ())): visit(m, stack + [m])
        state[n] = 2; order.append(n)
    sys.setrecursionlimit(10000)
    for n in sorted(graph): visit(n, [n])
    return order, cycles

def load(pdb_path, inventory):
    """Parse the PDB once: (streams, ipi, TypeDB, Generator, compiled game functions with their types)."""
    streams = msf_streams(pathlib.Path(pdb_path).read_bytes())
    ipi = tpi_records(streams[4])
    evidence, compiled = collections.defaultdict(set), {}
    for m in dbi_modules(streams[3]):
        if classify(m) != 'game' or m['stream'] == 0xffff: continue
        unit = pathlib.PureWindowsPath(m['obj'] or m['name']).stem
        for f in module_functions(streams[m['stream']], m['sym_bytes']):
            t = f['type']
            if f['type_is_id']:
                k, d = ipi.get(t, (0, b''))
                t = u32(d, 4) if k in (LF_FUNC_ID, LF_MFUNC_ID) else 0
            compiled.setdefault(f['name'], dict(unit=unit, type=t, size=f['size']))
            if t and '::' in f['name']: evidence[f['name'].rsplit('::', 1)[0]].add(t)
    db = TypeDB(tpi_records(streams[2]), evidence)
    fns = json.loads((pathlib.Path(inventory) / 'game_functions.json').read_text())
    roots = {n.rsplit('::', 1)[0] for n in fns if '::' in n}
    gen = Generator(db, {r for r in roots if gen_ok(r)})
    return streams, ipi, db, gen, compiled

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('pdb', type=pathlib.Path)
    ap.add_argument('--inventory', type=pathlib.Path, default=pathlib.Path('_local/pdb'))
    ap.add_argument('--out', type=pathlib.Path, default=pathlib.Path('recon/include'))
    args = ap.parse_args()
    streams, ipi, db, gen, _ = load(args.pdb, args.inventory)
    out = args.out; (out / 'compat').mkdir(parents=True, exist_ok=True)
    graph, all_checks, written = {}, [], {'fwd.h', 'all.h', 'free_functions.h', 'recon_all.h'}
    for top, ns in sorted(gen.tops.items()):
        text, inc, checks = gen.header(top, ns)
        write_if_changed(out / fname(top), text); written.add(fname(top))
        graph[top] = inc; all_checks += checks
    for old in out.glob('*.h'):
        if old.name not in written: old.unlink()
    write_if_changed(out / 'fwd.h', gen.fwd_h())
    write_if_changed(out / 'compat' / 'lib_types.h', gen.lib_types_h())
    order, cycles = toposort(graph)
    write_if_changed(out / 'all.h', '#pragma once\n' + ''.join('#include "%s"\n' % fname(t) for t in order))
    write_if_changed(out.parent / 'layout_check.cpp',
                     '// Generated: verifies reconstructed layouts against vagante.pdb (MSVC x86 ABI).\n'
                     '#define RECON_LAYOUT_CHECK 1\n#define private public\n#define protected public\n'
                     '#include "all.h"\n' + '\n'.join(all_checks) + '\n')
    report = dict(game_types=len(gen.game), headers=len(gen.tops), opaque=sorted(gen.opaque),
                  external_types=len(gen.ext), external_by_value=sum(e['byval'] for e in gen.ext.values()),
                  aliased_external=len(db.aliases), bad_template_heads=sorted(gen.bad_heads),
                  checks=len(all_checks), include_cycles=cycles[:50], warnings=dict(db.warn),
                  multi_definition_game_types={n: dict(chosen=hex(c['chosen']), candidates=[hex(t) for t in c['candidates']],
                                                       by_code_evidence=c['evidence'])
                                               for n, c in sorted(db.choices.items()) if n in gen.game})
    (args.inventory / 'headers_report.json').write_text(json.dumps(report, indent=1))
    print(json.dumps({k: (v if not isinstance(v, list) else len(v)) for k, v in report.items()}, indent=1))

def gen_ok(name): return not LIB_RX.search(name) and '<' not in name and '`' not in name

if __name__ == '__main__': main()
