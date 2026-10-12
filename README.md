# ⚡ shivrai-hon-core

Complete, self-contained blockchain node in C++17 — 10 modules, 40+ RPC
methods, PoA consensus, P2P gossip, and built-in web explorer.

## Build & Run

    bash scripts/build.sh
    bash scripts/run_node.sh

Browse: http://localhost:8545/

## Two-node P2P demo

    bash scripts/two_nodes.sh

## CLI

    ./build/shivrai-cli keygen
    ./build/shivrai-cli info
    ./build/shivrai-cli balance 0x...
    ./build/shivrai-cli call stablecoin_supply

## Docker

    docker compose up --build

## Benchmark

    make bench

## Docs

- WHITEPAPER.md
- docs/index.html
- CONTRIBUTING.md

## License
MIT — see LICENSE.
