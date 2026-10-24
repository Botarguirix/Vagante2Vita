"""Name raw offsets inside a reconstructed class, descending into embedded members.

    python tools/offset_name.py Client 0x875 0x3ec 0x1e04
    python tools/offset_name.py MainMenu 0xe4 0xe5 0xed

Uses the "// +0xNN" offsets of recon/include/*.h (verified against the PDB). Members whose type
is another reconstructed class are followed, so Client+0x875 resolves to gameEngine.<member>.
"""
import pathlib, re, sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from field_writes import members

INCLUDE = pathlib.Path('recon/include')

def class_header(name):
    top = name.split('::')[0]
    path = INCLUDE / ('%s.h' % re.sub(r'\W+', '_', top))
    return path if path.exists() else None

def nested_members(cls):
    """Members of a (possibly nested, Outer::Inner) class from its top-level header."""
    path = class_header(cls)
    if not path: return []
    return members(path, cls.split('::')[-1])

def resolve(cls, off, depth=0):
    fields = nested_members(cls)
    best = None
    for o, name, typ in fields:
        if o <= off: best = (o, name, typ)
    if not best: return '+0x%X' % off
    o, name, typ = best
    rest = off - o
    if rest == 0: return name
    base = re.sub(r'^(const |volatile )+', '', typ).strip()
    if depth < 6 and re.fullmatch(r'[A-Za-z_][\w:]*', base) and nested_members(base):
        return '%s.%s' % (name, resolve(base, rest, depth + 1))
    return '%s+0x%X' % (name, rest)

def main():
    cls = sys.argv[1]
    for arg in sys.argv[2:]:
        print('%s+%s = %s' % (cls, arg, resolve(cls, int(arg, 16) if arg.lower().startswith('0x') else int(arg))))

if __name__ == '__main__':
    main()
