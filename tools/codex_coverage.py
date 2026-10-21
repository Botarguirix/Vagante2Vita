"""Cross-reference the PDB function inventory with what the repo already references.

Counts a native function as "referenced" when its qualified name (Class::method)
appears in docs/, vita/src/, tools/ or tests/. Referenced is an upper bound on
reconstruction: a mention in an iteration note is not a port of the function.

    python tools/codex_coverage.py [--inventory _local/pdb]
"""
import argparse, collections, json, pathlib, re

def main():
    a = argparse.ArgumentParser()
    a.add_argument('--inventory', type=pathlib.Path, default=pathlib.Path('_local/pdb'))
    a.add_argument('--repo', type=pathlib.Path, default=pathlib.Path('.'))
    args = a.parse_args()
    fns = json.loads((args.inventory / 'game_functions.json').read_text())
    text = {'docs': '', 'code': ''}
    for sub, key in (('docs', 'docs'), ('vita/src', 'code'), ('tools', 'code'), ('tests', 'code')):
        for p in (args.repo / sub).rglob('*'):
            if p.suffix in ('.md', '.c', '.h', '.py', '.java', '.cpp') and p.name not in ('codex_coverage.py', 'pdb_inventory.py'):
                text[key] += p.read_text('utf-8', 'replace') + '\n'
    tokens = {k: set(re.findall(r'[A-Za-z_][\w]*(?:::~?[A-Za-z_]\w*)+', v)) for k, v in text.items()}
    words = {k: set(re.findall(r'[A-Za-z_]\w*', v)) for k, v in text.items()}
    hit = {}
    for name, f in fns.items():
        base = name.split('<')[0]
        where = [k for k in ('code', 'docs') if base in tokens[k]]
        hit[name] = where
    units = collections.defaultdict(lambda: dict(functions=0, bytes=0, ref_functions=0, ref_bytes=0, unit_named=False))
    for name, f in fns.items():
        u = units[f['unit']]; u['functions'] += 1; u['bytes'] += f['size']
        if hit[name]: u['ref_functions'] += 1; u['ref_bytes'] += f['size']
    for unit, u in units.items():
        u['unit_named'] = unit in words['code'] or unit in words['docs']
    total_b = sum(f['size'] for f in fns.values())
    ref = [n for n in fns if hit[n]]
    ref_code = [n for n in fns if 'code' in hit[n]]
    report = dict(
        game_functions=len(fns), game_bytes=total_b,
        referenced_functions=len(ref), referenced_bytes=sum(fns[n]['size'] for n in ref),
        referenced_in_code=len(ref_code), referenced_in_code_bytes=sum(fns[n]['size'] for n in ref_code),
        units_total=len(units), units_mentioned=sum(u['unit_named'] for u in units.values()),
        units_with_referenced_functions=sum(1 for u in units.values() if u['ref_functions']),
        units=dict(sorted(units.items(), key=lambda kv: -kv[1]['bytes'])),
        referenced=sorted(ref))
    (args.inventory / 'codex_coverage.json').write_text(json.dumps(report, indent=1))
    pct = lambda x, y: '%.1f%%' % (100.0 * x / y if y else 0)
    print('game functions: %d (%d bytes)' % (len(fns), total_b))
    print('named anywhere in repo: %d fns = %s of functions, %s of code bytes' % (
        len(ref), pct(len(ref), len(fns)), pct(report['referenced_bytes'], total_b)))
    print('named in code/tools/tests: %d fns, %s of code bytes' % (len(ref_code), pct(report['referenced_in_code_bytes'], total_b)))
    print('units mentioned: %d/%d; with referenced functions: %d' % (
        report['units_mentioned'], len(units), report['units_with_referenced_functions']))
    print('top units by size (ref fns / fns, ref bytes):')
    for unit, u in list(report['units'].items())[:45]:
        print('  %-22s %4d/%-4d %6s  %s' % (unit, u['ref_functions'], u['functions'], pct(u['ref_bytes'], u['bytes']), 'mentioned' if u['unit_named'] else '-'))

if __name__ == '__main__': main()
