#!/usr/bin/env bash
# Build and run the ScopeCore test suite against the in-process VISA simulator.
# No real VISA / hardware required.
#
# Prereqs: Qt 5 dev + qmake + g++. On Debian/Ubuntu:
#   sudo apt-get update && sudo apt-get install -y --no-install-recommends qtbase5-dev qtbase5-dev-tools
#
# Usage:  tests/run_tests.sh
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
BUILD="$HERE/build"

command -v qmake >/dev/null 2>&1 || { echo "qmake not found; install qtbase5-dev"; exit 1; }

rm -rf "$BUILD"
mkdir -p "$BUILD"
cd "$BUILD"
qmake "$HERE/tests.pro"
make -j"$(nproc)"
./tst_scope
