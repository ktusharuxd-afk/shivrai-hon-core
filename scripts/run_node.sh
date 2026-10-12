#!/bin/bash
set -e
cd "$(dirname "$0")/.."
[ -f build/shivrai-node ] || { echo "Run: bash scripts/build.sh"; exit 1; }
PORT="${1:-8545}"
exec ./build/shivrai-node "$PORT"
