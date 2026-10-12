#!/bin/bash
set -e
cd "$(dirname "$0")/.."
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release > /dev/null
make -j$(nproc) > /dev/null
echo "=== Running all tests ==="
ctest --output-on-failure
