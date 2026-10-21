#!/bin/bash
# Builds SFML 2.5.1 as static libraries for the PC reconstruction (MSYS2 UCRT64).
#   C:\msys64\usr\bin\bash.exe -lc "MSYSTEM=UCRT64 bash tools/build_sfml_pc.sh"
set -e
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
src="$root/_local/deps/SFML-2.5.1"
cmake -S "$src" -B "$src/build-pc" -G Ninja \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
  -DSFML_USE_SYSTEM_DEPS=ON -DSFML_BUILD_EXAMPLES=OFF -DSFML_BUILD_DOC=OFF \
  -DCMAKE_INSTALL_PREFIX="$root/_local/deps/sfml-pc"
cmake --build "$src/build-pc"
cmake --install "$src/build-pc"
ls "$root/_local/deps/sfml-pc/lib"
