#!/bin/bash
# Builds the reconstruction for PS Vita: _local/build-recon-vita/Vagante-ramaA.vpk
#   C:\msys64\usr\bin\bash.exe -lc "MSYSTEM=UCRT64 bash tools/build_recon_vita.sh"
# Needs VitaSDK with vdpm packages (default C:/vitasdk-pm) and the generated recon/include + recon/stubs.
set -e
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# Not ${VITASDK}: the machine-wide variable points at the older C:\vitasdk without vdpm packages.
export VITASDK="${RECON_VITASDK:-/c/vitasdk-pm}"
export PATH="$VITASDK/bin:$PATH"
build="$root/_local/build-recon-vita"
cmake -S "$root/recon" -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DVITA=ON \
  -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake" > "$build.configure.log" 2>&1 \
  || { tail -n 40 "$build.configure.log"; exit 1; }
set +e
cmake --build "$build" -- -k 0 > "$build.log" 2>&1
status=$?
grep -E 'error|FAILED|undefined reference' "$build.log" | head -n 60
if [ $status -ne 0 ]; then echo "BUILD FAILED (see $build.log)"; exit 1; fi
set -e
# Title id and version must match recon/CMakeLists.txt.
sys="$root/vita/sce_sys"
cd "$build"
vita-mksfoex -s APP_VER=00.10 -s TITLE_ID=VAGA00001 -d ATTRIBUTE2=12 "Vagante rama A" param.sfo
vita-pack-vpk -s param.sfo -b eboot.bin \
  -a "$sys/icon0.png=sce_sys/icon0.png" \
  -a "$sys/pic0.png=sce_sys/pic0.png" \
  -a "$sys/livearea/contents/bg0.png=sce_sys/livearea/contents/bg0.png" \
  -a "$sys/livearea/contents/startup.png=sce_sys/livearea/contents/startup.png" \
  -a "$sys/livearea/contents/template.xml=sce_sys/livearea/contents/template.xml" \
  Vagante-ramaA.vpk
ls -la "$build/Vagante-ramaA.vpk"
