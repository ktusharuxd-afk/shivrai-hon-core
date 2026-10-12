# Shivrai Hon Core — Whitepaper (v1.0)

## Abstract
A modular, single-binary blockchain node in C++17 with dual consensus
(Proof-of-Authority + Proof-of-Stake), ECDSA accounts, SQLite persistent
state, WASM smart contracts, JSON-RPC 2.0, P2P block gossip, and fourteen
domain modules — implemented in ~13,500 lines of clean C++.

## Architecture
- **Consensus:** Round-robin PoA (3s blocks) + stake-weighted PoS
- **Accounts:** ECDSA secp256k1, address = 0x + first 40 hex of SHA-256(pubkey)
- **Transactions:** signed over from|to|value|nonce|timestamp|method|params
- **Anti-replay:** per-account nonce enforced at submission
- **Balance check:** value + gas verified before mempool admission
- **State:** SQLite WAL, batched writes per block
- **Smart Contracts:** embedded wasm3 runtime with host functions
  (storage_get/set, emit_event, gas_charge, get_caller)
- **Networking:** HTTP POST /p2p/block gossip + startup chain sync
- **RPC:** JSON-RPC 2.0 + REST alias (GET /api/v1/rpc/<method>)

## Modules (14)
chain, stablecoin, lending, governance, token, NFT, AMM, staking,
multisig, p2p, pos, contract, explorer, consensus

## RPC Surface (52+ methods)
- chain: chain_info, chain_latest, chain_getBlock, account_balance, tx_submit, nonce_get
- stablecoin: mint, balance, supply
- lending: deposit, borrow, position
- governance: propose, vote, result
- token: deploy, info
- nft: mint, ownerOf, transfer, tokenURI, count
- amm: addLiquidity, reserves, swap
- staking: stake, unstake, info
- multisig: create, sign, info
- p2p: peers, addPeer
- pos: validator_register, validator_stake, validator_unstake,
       validator_info, validator_list, validator_total_stake, pos_leader
- contract: deploy, call, info, list, code
- explorer: stats

## Roadmap (Completed)
- ✅ Phase 1-4: Core, modules, web explorer, Docker/CI
- ✅ Phase 5: On-chain ECDSA verify + nonce replay protection
- ✅ Phase 6: P2P block gossip with startup sync
- ✅ Phase 7: SQLite batch writes
- ✅ Phase 8: Browser wallet UI
- ✅ Phase 9: WASM smart contracts (wasm3)
- ✅ Phase 10: REST API
- ✅ Phase 11: Proof-of-Stake consensus
- ✅ Phase 12: Documentation + GitHub Pages

## Future Work
- Crypto layer audit
- Public testnet + faucet
- Multi-validator PoS network demo
- WASM contract SDK for dapp developers
- Full ERC-20 reference implementation in WASM

## License
MIT
