# SHIVRAI HON Global Protocol — Phase 1 Foundation

## Release status

**Status: Foundation / documentation release.**

This phase establishes the SHIVRAI-specific capability registry and project roadmap without changing the existing mainnet consensus rules.

## Safety boundary

Phase 1 does **not** change:

- Proof-of-Work validation
- block interval rules
- block reward rules
- genesis data
- transaction/UTXO consensus rules
- existing wallet balances
- peer-network consensus behavior

The Phase 1 capability registry is informational and developer-facing.

## Implemented foundation

- HON Proof-of-Work foundation
- UTXO transaction model inherited from the repository base
- peer-to-peer payment foundation
- SHIVRAI protocol capability registry
- `getshivraifeatures` RPC
- SHIVRAI network/monetary foundation modules already present in the repository
- non-consensus treasury accounting foundation
- policy-level multisig/timelock foundation

## Planned / not consensus-active

- full smart-contract execution
- production EVM compatibility
- native token standards
- stablecoin primitives
- production DEX/AMM/lending
- staking/security modules
- RWA production framework
- NFT/game asset production standards
- gaming/GameFi execution
- decentralized identity production layer
- cross-chain bridge/message verification
- oracle and storage integrations
- governance activation
- privacy extensions
- enterprise/merchant production APIs

## Phase 1 RPC

```text
getshivraifeatures
```

Example:

```bash
bitcoin-cli getshivraifeatures
```

The response intentionally distinguishes `foundation` from `planned` capabilities.

## Treasury note

The treasury files included in this release are **foundation-only**. They do not receive network fees automatically and do not alter coin supply. Burn, treasury routing and multisig execution are future protocol work until an explicit consensus design and activation process is approved.

## Specification consistency

The repository currently implements a 5-minute block target and 2,000 HON reward profile in its SHIVRAI-specific foundation. The included whitepaper contains an older conflicting specification. This phase deliberately does not resolve that conflict by changing live consensus.

A future consensus upgrade must define one canonical specification, a dedicated testnet, activation conditions, compatibility/migration rules and rollback/emergency procedures before mainnet deployment.
