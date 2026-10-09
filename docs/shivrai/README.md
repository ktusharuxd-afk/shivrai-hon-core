# Shivrai Modules

Experimental, non-consensus modules for the SHIVRAI HON blockchain.

> ⚠️ **Status: Foundation** — All modules are **default-off**, **non-consensus**, and **experimental**. They do not affect the existing SHIVRAI HON network until a network upgrade activates them.

---

## 📋 Overview

| Module | Header | Purpose | Tests |
|--------|--------|---------|-------|
| Identity | `identity/identity.h` | DID registry, KYC, verifiable credentials | 8 |
| Interop | `interop/message.h` | Cross-chain message relay | 7 |
| RWA | `assets/rwa.h` | Real-world asset registry | 6 |
| Gaming | `gaming/assets.h` | Game asset registry | 7 |
| Payment | `fintech/payment.h` | Payment processor, invoices | 6 |
| Token Ledger | `assets/token_ledger.h` | Native token standard | (planned) |
| AMM | `defi/amm.h` | Automated market maker | (planned) |
| Treasury | `treasury/treasury.h` | Multisig, timelock, fee allocation | (planned) |
| EVM Compat | `evm_compat.h` | Solidity compatibility layer | (planned) |
| EVM State | `evm_state.h` | EVM state storage | (planned) |
| Monetary Policy | `monetary_policy.h` | Supply schedule | (planned) |
| Protocol Features | `protocol_features.h` | Feature registry | — |

**Current test coverage: 34 tests, all passing** ✅

---

## 🎯 Design Principles

1. **Non-consensus** — Modules do not affect Bitcoin Core consensus rules
2. **Default-off** — All features opt-in via runtime flags
3. **Bitcoin Core safe** — No modifications to existing Bitcoin files
4. **Thread-safe** — All registries use `std::recursive_mutex`
5. **Unit-tested** — Every public method has test coverage
6. **Standalone build** — Tests build independently of Bitcoin Core

---

## 🏗️ Architecture




    src/shivrai/
    ├── common/
    │   └── time.hpp              # Unix seconds, monotonic ns
    ├── identity/                 # DID, KYC, credentials
    ├── interop/                  # Cross-chain relay
    ├── assets/                   # RWA + token ledger
    ├── gaming/                   # Game assets
    ├── fintech/                  # Payment processor
    ├── defi/                     # AMM
    ├── treasury/                 # Multisig, timelock
    ├── evm_compat + evm_state
    ├── monetary_policy
    └── protocol_features

---

## 🔧 Common Utilities

### shivrai::common::time

All modules use centralized time helpers for consistency:

    #include "shivrai/common/time.hpp"

    uint64_t now_seconds();   // Unix timestamp in seconds
    uint64_t now_millis();    // Unix timestamp in milliseconds
    uint64_t monotonic_ns();  // Monotonic nanoseconds (RNG seeds)

**Why centralized?**
- Platform-independent
- Consistent units across modules
- Easy to mock in tests

---

## 📚 Module API Reference

### 1. Identity (shivrai::identity)

**Purpose:** Decentralized identifiers (DID), KYC records, verifiable credentials.

    IdentityRegistry reg;

    // Register a DID
    reg.register_did("did:shivrai:alice", "alice", "0xABCDEF...");

    // Issue a verifiable credential
    auto cred_id = reg.issue_credential(
        "did:shivrai:issuer",
        "did:shivrai:alice",
        "KYC", "verified",
        3600  // TTL seconds (0 = never expires)
    );

    // Verify
    bool ok = reg.verify_credential(cred_id);

    // KYC
    reg.set_kyc("did:shivrai:alice", "level2", "verifier1");

**Key types:**
- DID — id, controller, public_key, active
- Credential — id, issuer_did, subject_did, type, status
- KYCRecord — did, level, verified, verifier

---

### 2. Interop (shivrai::interop)

**Purpose:** Cross-chain message relay with multi-relayer consensus.

    MessageRelay relay(2);  // min 2 relayers

    auto msg_id = relay.send_message(
        ChainId::SHIVRAI, ChainId::ETHEREUM,
        "alice", "bob", "payload", 100, "HON"
    );

    // Relayers sign
    relay.relay_message(msg_id, "relayer1", "sig1");
    relay.relay_message(msg_id, "relayer2", "sig2");
    // -> status: RELAYED

    relay.confirm_message(msg_id);

    relay.on_confirmed([](const CrossChainMessage& m) {
        // Handle confirmed message
    });

**Lifecycle:** PENDING -> RELAYED -> CONFIRMED (or FAILED)

---

### 3. RWA (shivrai::assets)

**Purpose:** Real-world asset tokenization with legal compliance.

    RWARegistry reg;

    auto asset_id = reg.register_asset(
        "alice",
        RWAType::REAL_ESTATE,
        "Mumbai apartment",
        5000000, "USD", "issuer1", "IN",
        "sha256:legal-doc-hash"
    );

    reg.activate(asset_id);       // PENDING -> ACTIVE
    reg.transfer(asset_id, "bob", "tx-ref-123");
    reg.freeze(asset_id);
    reg.redeem(asset_id);

    auto history = reg.transfer_history(asset_id);

**Types:** REAL_ESTATE, INVOICE, COMMODITY, FUND, CERTIFICATE, OTHER
**Status:** PENDING, ACTIVE, FROZEN, REDEEMED

---

### 4. Gaming (shivrai::gaming)

**Purpose:** Game asset registry with rarity, levels, rewards.

    GameAssetRegistry reg;

    auto asset_id = reg.mint_asset(
        "player1", "game1", "sword", "Excalibur",
        AssetRarity::LEGENDARY,
        "ipfs://metadata-uri"
    );

    reg.transfer_asset(asset_id, "player2");
    reg.lock_asset(asset_id);
    reg.unlock_asset(asset_id);
    reg.level_up(asset_id, 2500);   // 2500 XP -> level 3
    reg.burn_asset(asset_id);

    reg.record_reward("player1", "game1", 100, "HON", "quest");

**Rarity:** COMMON, UNCOMMON, RARE, EPIC, LEGENDARY
**Status:** ACTIVE, LOCKED, BURNED
**Leveling:** 1 level per 1000 XP

---

### 5. Payment (shivrai::fintech)

**Purpose:** Payment processing with fees, invoices, batch, refunds.

    PaymentProcessor proc(100);  // 1% fee

    // Direct payment
    auto r = proc.send("alice", "bob", 1000, "HON", "memo");

    // Invoice
    auto inv = proc.create_invoice("merchant1", 5000, "HON", 3600, "order #123");
    auto pay_result = proc.pay_invoice(inv.id, "customer1");

    // Refund
    proc.refund(r.payment_id);

    // Callback (fires outside lock)
    proc.on_status_change([](const Payment& p) {
        // Handle status change
    });

**Fee:** (amount * fee_bps) / 10000
**Callbacks:** Fired outside mutex to prevent deadlocks

---

## 🔒 Thread Safety

All registries use `std::recursive_mutex` for thread-safe access.

**Why recursive?**
Methods like `is_active()` internally call `resolve()`. With a plain `std::mutex`, this would deadlock. `std::recursive_mutex` allows the same thread to re-acquire.

**Callback safety:**
Callbacks are invoked OUTSIDE the lock scope to prevent deadlocks.

    void notify(const Payment& p) {
        StatusCallback cb;
        {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            cb = callback_;
        }  // lock released
        if (cb) cb(p);  // outside lock
    }

---

## 🧪 Testing

Tests live in `tests/shivrai/` with a lightweight header-only framework.

**Run tests:**

    cd tests/shivrai
    cmake -B build -S .
    cmake --build build -j 4
    cd build
    ./test_identity
    ./test_interop
    ./test_rwa
    ./test_gaming
    ./test_payment

**Coverage:** 34 tests, all passing.

**CI:** Every push triggers GitHub Actions (.github/workflows/shivrai-tests.yml).

---

## 🛡️ Bitcoin Core Safety

**DO NOT MODIFY** (consensus-critical):

    src/util/time.cpp         # NodeClock epoch
    src/serialize.h           # Serialization format
    src/randomenv.cpp         # RNG entropy
    src/random.cpp            # FastRandomContext
    chainparams.cpp           # Network parameters

**Only modify:**
- src/shivrai/**       (this directory)
- tests/shivrai/**     (tests)

---

## 🗺️ Roadmap

    Phase 1 (Foundation)   DONE      Identity, Interop, RWA, Gaming, Payment
    Phase 2 (DeFi)         ACTIVE    AMM, Token Ledger, Treasury
    Phase 3 (EVM)          PLANNED   EVM Compat, EVM State
    Phase 4 (Advanced)     PLANNED   Lending, Stablecoin, Oracle, Governance
    Phase 5 (Tooling)      PLANNED   SDKs, Developer Portal, Monitoring

---

## 📞 Contact

- Repository: https://github.com/ktusharuxd-afk/shivrai-hon-core
- Website: https://ktusharuxd-afk.github.io/shivrai-hon-website/

---

*Last updated: October 2026*
