# ⚡ shivrai-hon-core

[![CI](https://github.com/ktusharuxd-afk/shivrai-hon-core/actions/workflows/ci.yml/badge.svg)](https://github.com/ktusharuxd-afk/shivrai-hon-core/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Docker](https://img.shields.io/badge/docker-ready-blue.svg)](Dockerfile)
[![GitHub release](https://img.shields.io/github/v/release/ktusharuxd-afk/shivrai-hon-core)](https://github.com/ktusharuxd-afk/shivrai-hon-core/releases)
[![Platform](https://img.shields.io/badge/platform-linux-lightgrey.svg)]()

A complete, self-contained blockchain node in **C++17** — PoA consensus,
ECDSA wallet, SQLite persistence, P2P gossip, 11 modules, 40+ RPC methods,
and a built-in web explorer. **~11,700 LOC** in a single repo.

## ✨ Features

- **Consensus:** Proof-of-Authority, 3-second blocks
- **Accounts:** secp256k1 ECDSA + on-chain signature verification
- **Anti-replay:** per-account nonce enforced at submission
- **State:** SQLite WAL with batched writes per block
- **RPC:** JSON-RPC 2.0 + REST alias /api/v1/rpc/&lt;method&gt;
- **P2P:** block gossip with startup chain sync
- **Modules:** chain, stablecoin, lending, governance, token, NFT, AMM, staking, multisig, p2p, explorer
- **Web UI:** explorer + browser wallet
- **Tooling:** Python SDK, CLI, benchmark suite
- **DevOps:** Docker multi-stage, GitHub Actions CI, Pages workflow

## 🚀 Quick Start

    git clone https://github.com/ktusharuxd-afk/shivrai-hon-core.git
    cd shivrai-hon-core
    bash scripts/build.sh
    bash scripts/run_node.sh

Then open http://localhost:8545/ for the explorer.

## 🌐 Two-node P2P demo

    bash scripts/two_nodes.sh

Starts Node A (validator, port 8545) and Node B (follower, port 8546),
both syncing to identical chain hashes.

## 🛠 CLI

    ./build/shivrai-cli keygen
    ./build/shivrai-cli info
    ./build/shivrai-cli balance 0x...
    ./build/shivrai-cli call stablecoin_supply

## 🐳 Docker

    docker compose up --build

## 📊 Benchmark

    make bench

## 📡 RPC Methods (40+)

| Module | Methods |
|--------|---------|
| chain | chain_info, chain_latest, chain_getBlock, account_balance, tx_submit, nonce_get |
| stablecoin | stablecoin_mint, stablecoin_balance, stablecoin_supply |
| lending | lending_deposit, lending_borrow, lending_position |
| governance | governance_propose, governance_vote, governance_result |
| token | token_deploy, token_info |
| nft | nft_mint, nft_ownerOf, nft_transfer, nft_tokenURI, nft_count |
| amm | amm_addLiquidity, amm_reserves, amm_swap |
| staking | staking_stake, staking_unstake, staking_info |
| multisig | multisig_create, multisig_sign, multisig_info |
| p2p | p2p_peers, p2p_addPeer |
| explorer | explorer_stats |

## 📚 Docs

- WHITEPAPER.md
- docs/index.html
- CONTRIBUTING.md

## 📄 License

MIT — see LICENSE.
