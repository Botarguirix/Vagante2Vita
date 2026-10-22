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
    ('include/SFML/OpenGL.hpp',
     '#if defined(SFML_SYSTEM_WINDOWS)',
     '#if defined(SFML_SYSTEM_VITA)\n\n    // vitaGL implements the desktop-style OpenGL API on the Vita GPU\n'
     '    #include <vitaGL.h>\n\n#elif defined(SFML_SYSTEM_WINDOWS)'),
    ('include/SFML/Window/WindowHandle.hpp',
     '#elif defined(SFML_SYSTEM_IOS)',
     '#elif defined(SFML_SYSTEM_VITA)\n\n    // The Vita has a single fullscreen display; no native window object\n'
     '    typedef void* WindowHandle;\n\n#elif defined(SFML_SYSTEM_IOS)'),
]

# The SDL backend (recon/sfml-sdl) takes precedence over the native one when SFML_USE_SDL is defined.
SDL_FIRST = '#if defined(SFML_SYSTEM_WINDOWS)'
for _rel, _hdr in (('src/SFML/Window/InputImpl.hpp', 'InputImpl'), ('src/SFML/Window/JoystickImpl.hpp', 'JoystickImpl'),
                   ('src/SFML/Window/SensorImpl.hpp', 'SensorImpl'), ('src/SFML/Window/ClipboardImpl.hpp', 'ClipboardImpl'),
                   ('src/SFML/Window/CursorImpl.hpp', 'CursorImpl')):
    PATCHES.append((_rel, SDL_FIRST, '#if defined(SFML_USE_SDL)\n    #include <SFML/Window/SDL/%s.hpp>\n#elif defined(SFML_SYSTEM_WINDOWS)' % _hdr))
PATCHES.append(('src/SFML/Window/WindowImpl.cpp', SDL_FIRST,
                '#if defined(SFML_USE_SDL)\n    #include <SFML/Window/SDL/WindowImplSDL.hpp>\n'
                '    typedef sf::priv::WindowImplSDL WindowImplType;\n#elif defined(SFML_SYSTEM_WINDOWS)'))
PATCHES.append(('src/SFML/Network/TcpListener.cpp', '#include <SFML/System/Err.hpp>\n',
                '#include <SFML/System/Err.hpp>\n\n#ifndef SOMAXCONN\n    #define SOMAXCONN 128 // newlib (Vita) lacks it\n#endif\n'))
PATCHES.append(('src/SFML/Window/GlContext.cpp', '#if !defined(SFML_OPENGL_ES)\n',
                '#if defined(SFML_USE_SDL)\n    #include <SFML/Window/SDL/SdlContext.hpp>\n'
                '    typedef sf::priv::SdlContext ContextType;\n#elif !defined(SFML_OPENGL_ES)\n'))

def main():
    ap = argparse.ArgumentParser(); ap.add_argument('root', type=pathlib.Path); args = ap.parse_args()
    for rel, anchor, replacement in PATCHES:
        p = args.root / rel; text = p.read_text(encoding='utf-8')
        if replacement in text: print('already patched:', rel); continue
        if anchor not in text: raise SystemExit('anchor not found in %s' % rel)
        p.write_text(text.replace(anchor, replacement, 1), encoding='utf-8'); print('patched:', rel)

if __name__ == '__main__': main()
