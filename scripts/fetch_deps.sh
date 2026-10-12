#!/usr/bin/env bash
# Fetch third-party deps (httplib.h + wasm3)
set -e
cd "$(dirname "$0")/.."
mkdir -p third_party

if [ ! -f third_party/httplib.h ]; then
  echo ">>> Fetching cpp-httplib..."
  curl -fsSL -o third_party/httplib.h \
    https://raw.githubusercontent.com/yhirose/cpp-httplib/v0.15.3/httplib.h
fi

if [ ! -d third_party/wasm3 ]; then
  echo ">>> Cloning wasm3..."
  git clone --depth 1 https://github.com/wasm3/wasm3.git third_party/wasm3
fi
