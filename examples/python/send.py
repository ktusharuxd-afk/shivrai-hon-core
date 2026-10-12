#!/usr/bin/env python3
"""Sign and submit a transfer tx using shivrai CLI."""
import sys, os, json, subprocess, time
sys.path.insert(0, os.path.join(os.path.dirname(__file__),"..","..","sdk","python"))
from shivrai import Client

URL = "http://127.0.0.1:8545"
c = Client(URL)

print("=== send demo ===")
# get validator address from node
info = c.chain_info()
print("chain:", info)

# note: use CLI for real signed tx (needs secp256k1)
print("\nSigned txs must be created with the C++ CLI:")
print("  ./build/shivrai-cli keygen")
print("  (then use shivrai-cli to craft tx — coming in phase 5.5)")
