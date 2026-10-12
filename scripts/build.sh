#!/usr/bin/env bash
set -e
cd "$(dirname "$0")/.."
bash scripts/fetch_deps.sh
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j"$(nproc)"
ctest --output-on-failure
