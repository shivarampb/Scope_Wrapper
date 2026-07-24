#!/usr/bin/env bash
# Build the shared VISA stub + ScopeCore + real plugin .so files and run the
# dynamic-load (QPluginLoader) test. No real VISA / hardware required.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
command -v qmake >/dev/null 2>&1 || { echo "qmake not found; install qtbase5-dev"; exit 1; }

rm -rf "$HERE/build"
mkdir -p "$HERE/build"

# Build order matters: stub -> core -> plugins -> test.
build_one() {
  local pro="$1"
  local obj="$HERE/build/obj_$(basename "$pro" .pro)"
  mkdir -p "$obj"
  ( cd "$obj" && qmake "$HERE/$pro" >/dev/null && make -j"$(nproc)" >/dev/null )
}

build_one visastub_lib.pro
build_one scopecore_stub.pro
build_one PluginKeysightDSOX2012A.pro
build_one PluginTektronixMDO34.pro
build_one PluginLeCroyWaveSurfer42XS.pro
build_one tst_manager_dl.pro

export LD_LIBRARY_PATH="$HERE/build/lib:${LD_LIBRARY_PATH:-}"
export SCOPE_PLUGIN_DIR="$HERE/build/bin/plugins"
"$HERE/build/tst_manager_dl"
