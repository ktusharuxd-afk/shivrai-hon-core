// Copyright (c) 2026-present The SHIVRAI HON Core developers
// Distributed under the MIT software license.
#ifndef SHIVRAI_HON_EVM_COMPAT_H
#define SHIVRAI_HON_EVM_COMPAT_H

#include <array>
#include <cstdint>
#include <string>

namespace shivrai::evm {

using Address = std::array<uint8_t, 20>;

// EVM compatibility contract.
// Execution is deferred until state/account model, fee market, and
// activation rules are reviewed.
struct EVMNetworkConfig {
    uint64_t chain_id;
    uint64_t default_gas_limit;
    uint64_t target_gas_per_block;
    uint64_t max_gas_per_block;
    uint64_t gas_price_floor_hon_nano;
    bool execution_live;
};

// EIP-1559-style transaction envelope.
struct EVMTransactionEnvelope {
    uint64_t chain_id;
    uint64_t nonce;
    Address to;
    uint64_t value_hon_nano;
    uint64_t gas_limit;
    uint64_t max_fee_per_gas_nano;
    uint64_t max_priority_fee_per_gas_nano;
};

const EVMNetworkConfig& GlobalEVMNetworkConfig();
bool ValidateEnvelope(const EVMTransactionEnvelope& tx, std::string& error);
const char* EVMExecutionStatus();

// --- New: Address utilities ---

// Zero address (0x0000...0000).
Address ZeroAddress();

// All-bytes-0xFF address (useful for burn addresses).
Address MaxAddress();

// Check if address is the zero address.
bool IsZeroAddress(const Address& addr);

// Convert address to hex string "0x..." (42 chars total).
std::string AddressToHex(const Address& addr);

// Parse "0x..." hex string (42 chars). Returns false on invalid input.
bool HexToAddress(const std::string& hex, Address& out, std::string& error);

// --- New: Fee market helpers ---

// Effective gas price = min(max_fee, base_fee + priority_fee).
// For non-EIP-1559, use max_fee directly.
uint64_t EffectiveGasPrice(const EVMTransactionEnvelope& tx, uint64_t base_fee_nano);

// Max total cost = gas_limit * max_fee_per_gas + value.
// Returns 0 on overflow.
uint64_t MaxTotalCost(const EVMTransactionEnvelope& tx);

// Tip paid to miner = effective_price - base_fee (capped to priority).
uint64_t MinerTip(const EVMTransactionEnvelope& tx, uint64_t base_fee_nano);

// --- New: Validation helpers ---

// Validate chain_id matches network config.
bool ValidateChainId(uint64_t chain_id, std::string& error);

// Validate gas limit is within network bounds.
bool ValidateGasLimit(uint64_t gas_limit, std::string& error);

// Validate fee structure (priority <= max, >= floor).
bool ValidateFees(const EVMTransactionEnvelope& tx, std::string& error);

// --- New: Description ---

// Human-readable config summary (for logging/RPC).
std::string DescribeEVMConfig();

} // namespace shivrai::evm
#endif // SHIVRAI_HON_EVM_COMPAT_H
