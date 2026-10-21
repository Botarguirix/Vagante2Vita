"""Apply the PS Vita platform patches to an SFML 2.5.1 source tree (idempotent).

    python tools/patch_sfml_vita.py _local/deps/SFML-2.5.1

Only platform detection lives here for now; the Window/System backends for Vita
(vitaGL context, SceCtrl input) will be added as new source files.
"""
import argparse, pathlib

PATCHES = [
    ('include/SFML/Config.hpp',
     '#elif defined(__unix__)\n',
     '#elif defined(__vita__)\n\n    // PS Vita (VitaSDK); not __unix__\n    #define SFML_SYSTEM_VITA\n\n'
     '#elif defined(__unix__)\n'),
    ('include/SFML/Window/WindowHandle.hpp',
     '#elif defined(SFML_SYSTEM_IOS)',
     '#elif defined(SFML_SYSTEM_VITA)\n\n    // The Vita has a single fullscreen display; no native window object\n'
     '    typedef void* WindowHandle;\n\n#elif defined(SFML_SYSTEM_IOS)'),
]

def main():
    ap = argparse.ArgumentParser(); ap.add_argument('root', type=pathlib.Path); args = ap.parse_args()
    for rel, anchor, replacement in PATCHES:
        p = args.root / rel; text = p.read_text(encoding='utf-8')
        if replacement in text: print('already patched:', rel); continue
        if anchor not in text: raise SystemExit('anchor not found in %s' % rel)
        p.write_text(text.replace(anchor, replacement, 1), encoding='utf-8'); print('patched:', rel)

if __name__ == '__main__': main()
