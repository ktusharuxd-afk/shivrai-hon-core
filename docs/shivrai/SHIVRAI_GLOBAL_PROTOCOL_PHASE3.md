# SHIVRAI HON Global Blockchain — Phase 3

## Scope

Phase 3 establishes a deterministic protocol foundation for the dedicated SHIVRAI network without silently replacing the existing Bitcoin-derived mainnet parameters.

### Added
- Canonical SHIVRAI genesis specification.
- Explicit genesis message and candidate PoW fields.
- Genesis activation lock until a reproducible launch ceremony.
- Deterministic HON block-subsidy policy.
- 5-minute target block interval.
- 2,000 HON initial block reward.
- 210,000-block halving interval.
- Automated unit tests for the Phase 3 policy.

## Genesis safety

The genesis nonce/time and final hash are deliberately **not** hard-coded as a live mainnet genesis yet. The launch team must first freeze a timestamp, select a launch difficulty based on expected network security, mine the genesis deterministically, publish the complete block/header/transaction material, and independently verify its hash.

This avoids creating a chain that appears production-ready while its genesis proof-of-work has not been independently reproduced.

## Monetary policy

The current candidate policy is:

- Initial subsidy: 2,000 HON/block
- Target interval: 300 seconds
- Halving interval: 210,000 blocks
- Supply model: declining subsidy; not declared fixed-supply

Changing these values after activation would be a consensus change and must require an explicit protocol version/activation process.

## AI policy

AI is intentionally excluded from the SHIVRAI protocol. No AI execution, AI-agent transaction layer, or AI consensus dependency is introduced by Phase 3.
