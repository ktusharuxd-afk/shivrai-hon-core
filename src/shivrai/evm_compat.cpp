// Copyright (c) 2026-present The SHIVRAI HON Core developers
// Distributed under the MIT software license.
#include <shivrai/evm_compat.h>

#include <limits>

namespace shivrai::evm {

const EVMNetworkConfig& GlobalEVMNetworkConfig()
{
    // Provisional chain ID. It must be registered/frozen before mainnet EVM
    // activation. No EVM transaction is consensus-valid while execution_live=false.
    static const EVMNetworkConfig config{
        2026001,  // provisional SHIVRAI HON EVM chain ID
        30'000'000,
        15'000'000,
        30'000'000,
        1,
        false,
    };
    return config;
}

bool ValidateEnvelope(const EVMTransactionEnvelope& tx, std::string& error)
{
    const auto& cfg = GlobalEVMNetworkConfig();
    if (tx.chain_id != cfg.chain_id) {
        error = "wrong chain id";
        return false;
    }
    if (tx.gas_limit == 0 || tx.gas_limit > cfg.max_gas_per_block) {
        error = "gas limit outside SHIVRAI bounds";
        return false;
    }
    if (tx.max_priority_fee_per_gas_nano > tx.max_fee_per_gas_nano) {
        error = "priority fee exceeds max fee";
        return false;
    }
    if (tx.max_fee_per_gas_nano < cfg.gas_price_floor_hon_nano) {
        error = "max fee below SHIVRAI gas-price floor";
        return false;
    }
    return true;
}

const char* EVMExecutionStatus()
{
    return GlobalEVMNetworkConfig().execution_live ? "live" : "foundation-only";
}

} // namespace shivrai::evm
