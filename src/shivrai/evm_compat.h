// Copyright (c) 2026-present The SHIVRAI HON Core developers
// Distributed under the MIT software license.
#ifndef SHIVRAI_HON_EVM_COMPAT_H
#define SHIVRAI_HON_EVM_COMPAT_H

#include <array>
#include <cstdint>
#include <string>

namespace shivrai::evm {

// Phase 4 establishes the deterministic EVM compatibility contract. It does
// not embed an EVM interpreter yet; execution is deliberately deferred until
// the state/account model, fee market, and activation rules are reviewed.
struct EVMNetworkConfig {
    uint64_t chain_id;
    uint64_t default_gas_limit;
    uint64_t target_gas_per_block;
    uint64_t max_gas_per_block;
    uint64_t gas_price_floor_hon_nano;
    bool execution_live;
};

struct EVMTransactionEnvelope {
    uint64_t chain_id;
    uint64_t nonce;
    std::array<uint8_t, 20> to;
    uint64_t value_hon_nano;
    uint64_t gas_limit;
    uint64_t max_fee_per_gas_nano;
    uint64_t max_priority_fee_per_gas_nano;
};

const EVMNetworkConfig& GlobalEVMNetworkConfig();
bool ValidateEnvelope(const EVMTransactionEnvelope& tx, std::string& error);
const char* EVMExecutionStatus();

} // namespace shivrai::evm
#endif // SHIVRAI_HON_EVM_COMPAT_H
