#!/usr/bin/env bash
set -e
cd "$(dirname "$0")/.."
mkdir -p data
pkill -f shivrai-node 2>/dev/null || true
sleep 1

echo ">>> Node A (validator, port 8545)..."
rm -f data/a.db data/a.db-wal data/a.db-shm
./build/shivrai-node --db data/a.db --port 8545 > data/a.log 2>&1 &
echo $! > data/a.pid
sleep 3

echo ">>> Node B (follower, port 8546) -> peer A..."
rm -f data/b.db data/b.db-wal data/b.db-shm
./build/shivrai-node --db data/b.db --port 8546 --peer http://127.0.0.1:8545 --novalidate > data/b.log 2>&1 &
echo $! > data/b.pid
sleep 6

echo
echo ">>> chain_info A:"; curl -sf http://127.0.0.1:8545/api/v1/rpc/chain_info; echo
echo ">>> chain_info B:"; curl -sf http://127.0.0.1:8546/api/v1/rpc/chain_info; echo
echo ">>> peers on B:";   curl -sf http://127.0.0.1:8546/api/v1/rpc/p2p_peers; echo
echo
echo ">>> Watching 8s for gossip propagation..."
sleep 8
HA=$(curl -sf http://127.0.0.1:8545/api/v1/rpc/chain_info | python3 -c "import sys,json;print(json.load(sys.stdin)['result']['height'])")
HB=$(curl -sf http://127.0.0.1:8546/api/v1/rpc/chain_info | python3 -c "import sys,json;print(json.load(sys.stdin)['result']['height'])")
echo ">>> Node A height: $HA"
echo ">>> Node B height: $HB"
if [ "$HA" = "$HB" ] && [ "$HA" != "0" ]; then
  echo "✅ P2P SYNC WORKING — both nodes at height $HA"
else
  echo "⚠️  heights differ — check data/a.log and data/b.log"
fi
echo
echo ">>> Stop: kill \$(cat data/a.pid) \$(cat data/b.pid)"
