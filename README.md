# SHIVRAI HON (HON)

![Shivrai Tests](https://github.com/ktusharuxd-afk/shivrai-hon-core/actions/workflows/shivrai-tests.yml/badge.svg)

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

**Note:** The `chainparams.cpp` has been corrected to reflect the live mainnet value (8 hours, 28800 seconds). The network has operated at this block target since genesis (June 2026). Any future consensus parameter changes require a separately specified network upgrade, activation plan, testnet validation and migration procedure.

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

## Shivrai modules (foundation, default-off, non-consensus)

Experimental blockchain modules for payments, DeFi, fintech, gaming, digital
assets, identity and cross-chain interoperability. All default-off and
non-consensus — they do not affect the existing SHIVRAI HON network.

| Module | Purpose | Tests |
|--------|---------|-------|
| Identity | DID registry, KYC, credentials | 8 |
| Interop | Cross-chain message relay | 7 |
| RWA | Real-world asset registry | 6 |
| Gaming | Game asset registry | 7 |
| Payment | Payments, invoices, refunds | 6 |
| Token Ledger | Native token standard | foundation |
| AMM | Automated market maker | foundation |
| Treasury | Multisig, timelock | foundation |
| EVM Compat | Solidity compatibility | foundation |
| Monetary Policy | Supply schedule | foundation |

**Test coverage:** 34 tests, all passing ✅

**Documentation:** [docs/shivrai/README.md](docs/shivrai/README.md)

**Test suite:** [tests/shivrai/README.md](tests/shivrai/README.md)

**CI:** Every push to `src/shivrai/**` or `tests/shivrai/**` triggers
[Shivrai Tests](.github/workflows/shivrai-tests.yml) — independent from
Bitcoin Core CI, runs in ~30 seconds.

**Design principles:**

1. Non-consensus — no changes to Bitcoin Core consensus rules
2. Default-off — all features opt-in via runtime flags
3. Bitcoin Core safe — no modifications to consensus-critical files
4. Thread-safe — all registries use `std::recursive_mutex`
5. Unit-tested — every public method has test coverage
6. Standalone build — tests build independently of Bitcoin Core
