#!/bin/bash
# Builds the reconstruction for PC (MSYS2 UCRT64 GCC + static SFML 2.5.1).
#   C:\msys64\usr\bin\bash.exe -lc "MSYSTEM=UCRT64 bash tools/build_recon_pc.sh"
# Prerequisites: tools/build_sfml_pc.sh, tools/pdb_headers.py and tools/pdb_stubs.py.
set -e
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="$root/_local/build-recon-pc"
cmake -S "$root/recon" -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DSFML_DIR="$root/_local/deps/sfml-pc/lib/cmake/SFML" > "$build.configure.log" 2>&1 \
  || { tail -n 30 "$build.configure.log"; exit 1; }
set +e
cmake --build "$build" -- -k 0 > "$build.log" 2>&1
status=$?
grep -E 'error|FAILED' "$build.log" | head -n 60
if [ $status -ne 0 ]; then echo "BUILD FAILED (see $build.log)"; exit 1; fi
echo "OK: $build/vagante.exe"
