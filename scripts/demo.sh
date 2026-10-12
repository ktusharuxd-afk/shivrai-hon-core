#!/usr/bin/env bash
set -e
cd "$(dirname "$0")/.."
mkdir -p data
./build/shivrai-node --db data/shivrai.db --host 0.0.0.0 --port 8545 &
PID=$!
trap "kill $PID 2>/dev/null || true; wait $PID 2>/dev/null || true" EXIT

echo ">>> Waiting for RPC..."
for i in $(seq 1 40); do
  if curl -sf http://127.0.0.1:8545/health >/dev/null 2>&1; then
    echo ">>> Ready after ${i} attempts"
    break
  fi
  sleep 0.25
done

python3 examples/python/demo.py
