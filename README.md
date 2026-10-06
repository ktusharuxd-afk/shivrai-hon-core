# SHIVRAI HON (HON)

SHIVRAI HON is an independent decentralized blockchain project built from a Bitcoin Core-derived codebase. The long-term direction is a modular global blockchain for payments, smart contracts, fintech, DeFi, gaming, digital assets, identity and cross-chain interoperability.

## Current live-chain safety baseline

This repository release is intentionally **non-consensus-changing** for the existing SHIVRAI HON network.

| Parameter | Current repository value |
|---|---|
| Coin | SHIVRAI HON |
| Symbol | HON |
| Distribution | Mining-based |
| Block reward | 2,000 HON (current repository implementation) |
| Block target | **8 hours** (28800 seconds — live mainnet since genesis, June 2026) |
| Consensus foundation | Proof of Work |

**Important:** The included whitepaper contains an older/conflicting specification mentioning an 8-hour block target and a different emission schedule. This release does **not** silently change live consensus to match that document. Any future consensus change requires a separately specified network upgrade, activation plan, testnet validation and migration procedure.

## Phase 1 documentation

The following Phase 1 documents are included in this ZIP:

- `docs/SHIVRAI_GLOBAL_PROTOCOL_PHASE1.md`
- `docs/SHIVRAI_GLOBAL_PROTOCOL_ROADMAP.md`
- `docs/BUILD_VALIDATION.md`

`getshivraifeatures` is the Phase 1 capability-registry RPC. It distinguishes current foundation capabilities from planned work so external tools do not mistake roadmap items for live protocol features.

## Treasury foundation — default off

This release includes a **non-consensus treasury foundation** under:

- `src/shivrai/treasury/treasury.h`
- `src/shivrai/treasury/treasury.cpp`
- `src/test/shivrai_treasury_tests.cpp`
- `docs/SHIVRAI_TREASURY_AND_MULTISIG.md`

The foundation provides deterministic fee-allocation/accounting helpers and a policy-level multisig/timelock model for future development. It is **not wired into block validation, UTXO accounting, fee collection, burn rules, or mainnet consensus** in this release.

Therefore adding these files does not change how existing SHIVRAI HON blocks are validated or how current miners operate.

## Global protocol direction

Planned modules include:

- Payments and settlement
- Smart contracts / EVM-compatible execution
- Stablecoins
- DEX / AMM / lending
- Native assets and NFTs
- Gaming / GameFi
- RWA and tokenization
- Decentralized identity
- Cross-chain interoperability
- Oracles and storage integrations
- Developer SDK / RPC infrastructure
- Governance
- Enterprise and merchant infrastructure

Each module remains subject to implementation, tests, security review and an explicit activation plan before being treated as a live consensus feature.

## AI policy

**AI is intentionally excluded from the SHIVRAI HON protocol.** The blockchain does not embed AI agents, AI consensus, AI validators or AI transaction execution.

External applications may use AI independently while interacting with SHIVRAI.

## Status terminology

- **Foundation** — code/documentation exists but does not necessarily mean production-ready or consensus-active.
- **Planned** — roadmap item; not live.
- **Consensus-active** — explicitly activated by the network specification and deployment process.

## License

Distributed under the MIT software license.
