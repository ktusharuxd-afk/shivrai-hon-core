# Shivrai Hon Core — Whitepaper (v1.0)

## Abstract
A modular single-binary blockchain node in C++17 with PoA consensus,
ECDSA accounts, SQLite persistent state, JSON-RPC 2.0, P2P block gossip,
and ten domain modules.

## Architecture
- Consensus: Round-robin PoA, blocks every 3s
- Accounts: ECDSA secp256k1, address = 0x + first 40 hex of SHA-256(pubkey)
- Transactions: signed over from|to|value|nonce|timestamp|method|params
- Anti-replay: per-account nonce enforced at submission
- State: SQLite WAL, batched writes per block
- Networking: HTTP POST /p2p/block gossip, peers via --peer
- REST alias: GET /api/v1/rpc/<method>?params={...}

## Modules
chain, stablecoin, lending, governance, token, NFT, AMM, staking, multisig, p2p

## Roadmap
- Phase 9: WASM smart contracts (wasm3)
- Phase 11: Proof-of-Stake
- Crypto layer audit
- Public testnet + faucet

## License
MIT
