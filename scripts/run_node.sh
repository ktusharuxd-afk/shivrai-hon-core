#!/usr/bin/env bash
set -e
cd "$(dirname "$0")/.."
mkdir -p data
exec ./build/shivrai-node --db data/shivrai.db --port 8545 "$@"
