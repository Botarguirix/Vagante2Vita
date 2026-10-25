#!/usr/bin/env python3
"""Compile/run host bridge tests without running commercial game code.

  python -I tests/host/run_tests.py [stubs|openal|gl|xinput|vita_gl_compat ...]
OpenAL uses its null backend; GL uses a hidden, real SDL OpenGL context.
"""
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
GCC = Path(r"C:\msys64\ucrt64\bin\gcc.exe")
ENV = dict(os.environ, PATH=str(GCC.parent) + os.pathsep + os.environ.get("PATH", ""), ALSOFT_DRIVERS="null")
GROUPS = {
    "stubs": {"steam_api.dll", "WS2_32.dll", "WLDAP32.dll", "ADVAPI32.dll", "CRYPT32.dll", "Normaliz.dll", "DINPUT8.dll", "XINPUT9_1_0.dll", "VERSION.dll"},
    "openal": {"OpenAL32.dll"},
    "gl": {"OPENGL32.dll"},
    "xinput": {"XINPUT9_1_0.dll"},
    "vita_gl_compat": set(),
}
LIBS = {"vita_gl_compat": ["-DVAGANTE_GL_COMPAT_TEST", "-lSDL2", "-lopengl32"], "stubs": ["-lSDL2", "-lpthread"], "xinput": ["-lSDL2", "-lpthread"], "openal": ["-DAL_LIBTYPE_STATIC", "-static", "-lopenal", "-lstdc++", "-lole32", "-lwinmm", "-luuid", "-lavrt", "-lpthread"],
        "gl": ["-lSDL2", "-lopengl32", "-lpthread"]}

def coverage(unit):
    if unit == "vita_gl_compat": return
    source_unit = "stubs" if unit == "xinput" else unit
    source = (ROOT / f"recomp/host/{source_unit}.c").read_text(encoding="utf-8-sig")
    table = source.split(f"const ApiEntry win32_{source_unit}[]", 1)[1]
    names = set(re.findall(r"\bE\((\w+)\)", table))
    names.update(re.findall(r'\{\s*"([^\"]+)"', table))
    names.update("WLDAP32.dll#" + n for n in re.findall(r"\bLDAP\((\d+),", table))
    wanted = set()
    dll = None
    for line in (ROOT / "artifacts/imports.txt").read_text().splitlines():
        if line and not line[0].isspace():
            dll = line.split()[0]
        elif dll in GROUPS[unit]:
            for name in line.split():
                if unit == "gl" and name.startswith("wgl"):
                    continue
                wanted.add(dll + name if name.startswith("#") else name)
    assert wanted, "empty imports inventory"
    assert not wanted - names, f"missing {unit}: {sorted(wanted - names)}"
    print(f"{unit}: all {len(wanted)} requested imports registered", flush=True)


def main():
    units = sys.argv[1:] or [unit for unit in GROUPS if (ROOT / f"tests/host/{unit}_test.c").exists()]
    for unit in units:
        if unit not in GROUPS:
            raise SystemExit(f"unknown unit: {unit}")
        coverage(unit)
        out = ROOT / f"artifacts/{unit}_test.exe"
        out.parent.mkdir(parents=True, exist_ok=True)
        command = [str(GCC), "-std=gnu11", "-Wall", "-Wextra", "-Wno-unused-parameter",
                   "-Irecomp/runtime", "-Irecomp/host", f"tests/host/{unit}_test.c",
                   str(ROOT / ("recomp/vita/gl_compat.c" if unit == "vita_gl_compat" else f"recomp/host/{'stubs' if unit == 'xinput' else unit}.c")), *LIBS[unit], "-o", str(out)]
        subprocess.run(command, cwd=ROOT, env=ENV, check=True)
        subprocess.run([str(out)], cwd=ROOT, env=ENV, check=True, timeout=60)


if __name__ == "__main__":
    main()
