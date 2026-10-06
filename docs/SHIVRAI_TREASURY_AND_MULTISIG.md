# SHIVRAI HON Treasury & Multisig Foundation

## Purpose

This module is a **future-protocol foundation only**. It provides deterministic accounting and policy helpers so the eventual treasury design can be tested independently of live consensus.

## Current status

**Default-off / non-consensus / not mainnet-active.**

The current code does not:

- intercept transaction fees;
- modify block validation;
- modify UTXO balances;
- burn HON from circulation;
- move real funds;
- create or spend native multisig outputs;
- change the existing live chain.

## Fee allocation policy model

The default policy object demonstrates the previously discussed accounting split:

- 50% security
- 20% burn accounting
- 15% ecosystem
- 10% developer
- 5% reserve

This is a **policy example**, not a live monetary rule. Changing it has no effect on the current network because the module is not wired into consensus.

## Multisig policy model

The foundation supports:

- a configured signer set;
- an M-of-N approval threshold;
- duplicate-signer rejection;
- proposal approval tracking;
- block-height timelock readiness.

It is not a replacement for Bitcoin Script/PSBT native multisig.

## Future integration requirements

Before this module can become a live protocol feature, SHIVRAI must separately specify and test:

1. fee collection source;
2. exact on-chain treasury output/account model;
3. burn semantics and supply accounting;
4. native multisig transaction/script format;
5. replay and authorization rules;
6. persistent state and reorg behavior;
7. consensus activation height/versioning;
8. testnet and migration procedure;
9. audit/fuzz/property testing;
10. operational key management and emergency controls.

No such integration is activated by this release.
