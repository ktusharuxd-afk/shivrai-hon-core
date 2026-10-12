#!/bin/bash
URL="${1:-http://localhost:8545}"
echo "=== shivrai-node curl demo ==="
echo "--- GET /health ---"
curl -s "$URL/health"; echo
echo "--- POST /rpc system.health ---"
curl -s -X POST "$URL/rpc" -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"system.health","params":[],"id":1}'; echo
echo "--- POST /rpc system.echo ---"
curl -s -X POST "$URL/rpc" -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"system.echo","params":["hello"],"id":2}'; echo
echo "done"
