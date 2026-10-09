// Copyright (c) 2026-present The SHIVRAI HON Core developers
// Distributed under the MIT software license.
#include <shivrai/evm_compat.h>

#include <limits>
#include <sstream>
#include <iomanip>

namespace shivrai::evm {

const EVMNetworkConfig& GlobalEVMNetworkConfig()
{
    static const EVMNetworkConfig config{
        2026001,
        30'000'000,
        15'000'000,
        30'000'000,
        1,
        false,
    };
    return config;
}

bool ValidateChainId(uint64_t chain_id, std::string& error)
{
    const auto& cfg = GlobalEVMNetworkConfig();
    if (chain_id != cfg.chain_id) {
        error = "wrong chain id";
        return false;
    }
    return true;
}

bool ValidateGasLimit(uint64_t gas_limit, std::string& error)
{
    const auto& cfg = GlobalEVMNetworkConfig();
    if (gas_limit == 0) {
        error = "gas limit must be > 0";
        return false;
    }
    if (gas_limit > cfg.max_gas_per_block) {
        error = "gas limit exceeds block max";
        return false;
    }
    return true;
}

bool ValidateFees(const EVMTransactionEnvelope& tx, std::string& error)
{
    const auto& cfg = GlobalEVMNetworkConfig();
    if (tx.max_priority_fee_per_gas_nano > tx.max_fee_per_gas_nano) {
        error = "priority fee exceeds max fee";
        return false;
    }
    if (tx.max_fee_per_gas_nano < cfg.gas_price_floor_hon_nano) {
        error = "max fee below gas-price floor";
        return false;
    }
    return true;
}

bool ValidateEnvelope(const EVMTransactionEnvelope& tx, std::string& error)
{
    if (!ValidateChainId(tx.chain_id, error)) return false;
    if (!ValidateGasLimit(tx.gas_limit, error)) return false;
    if (!ValidateFees(tx, error)) return false;
    return true;
}

// --- Address utilities ---

Address ZeroAddress()
{
    Address a{};
    a.fill(0);
    return a;
}

Address MaxAddress()
{
    Address a{};
    a.fill(0xFF);
    return a;
}

bool IsZeroAddress(const Address& addr)
{
    for (auto b : addr) {
        if (b != 0) return false;
    }
    return true;
}

std::string AddressToHex(const Address& addr)
{
    std::ostringstream ss;
    ss << "0x" << std::hex << std::setfill('0');
    for (auto b : addr) {
        ss << std::setw(2) << static_cast<unsigned>(b);
    }
    return ss.str();
}

// --- Fee market helpers ---

uint64_t EffectiveGasPrice(const EVMTransactionEnvelope& tx, uint64_t base_fee_nano)
{
    const uint64_t cap = base_fee_nano + tx.max_priority_fee_per_gas_nano;
    return (cap < tx.max_fee_per_gas_nano) ? cap : tx.max_fee_per_gas_nano;
}

uint64_t MaxTotalCost(const EVMTransactionEnvelope& tx)
{
    if (tx.gas_limit == 0) return 0;
    const uint64_t max_gas_cost = std::numeric_limits<uint64_t>::max() / tx.gas_limit;
    if (tx.max_fee_per_gas_nano > max_gas_cost) return 0;  // overflow
    const uint64_t gas_cost = tx.gas_limit * tx.max_fee_per_gas_nano;
    if (gas_cost > std::numeric_limits<uint64_t>::max() - tx.value_hon_nano) return 0;
    return gas_cost + tx.value_hon_nano;
}

uint64_t MinerTip(const EVMTransactionEnvelope& tx, uint64_t base_fee_nano)
{
    const uint64_t eff = EffectiveGasPrice(tx, base_fee_nano);
    if (eff <= base_fee_nano) return 0;
    const uint64_t tip = eff - base_fee_nano;
    return (tip > tx.max_priority_fee_per_gas_nano) ? tx.max_priority_fee_per_gas_nano : tip;
}

const char* EVMExecutionStatus()
{
    return GlobalEVMNetworkConfig().execution_live ? "live" : "foundation-only";
}

std::string DescribeEVMConfig()
{
    const auto& cfg = GlobalEVMNetworkConfig();
    std::ostringstream ss;
    ss << "SHIVRAI HON EVM Config:\n"
       << "  Chain ID: " << cfg.chain_id << "\n"
       << "  Default gas limit: " << cfg.default_gas_limit << "\n"
       << "  Target gas/block: " << cfg.target_gas_per_block << "\n"
       << "  Max gas/block: " << cfg.max_gas_per_block << "\n"
       << "  Gas price floor: " << cfg.gas_price_floor_hon_nano << " nano-HON\n"
       << "  Execution status: " << EVMExecutionStatus();
    return ss.str();
}

bool HexToAddress(const std::string& hex, Address& out, std::string& error)
{
    std::string s = hex;
    if (s.size() >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s = s.substr(2);
    }
    if (s.size() != 40) {
        error = "address hex must be 40 characters (without 0x prefix)";
        return false;
    }
    for (size_t i = 0; i < 40; i += 2) {
        char hi = s[i];
        char lo = s[i + 1];
        auto hexval = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };
        int h = hexval(hi);
        int l = hexval(lo);
        if (h < 0 || l < 0) {
            error = "invalid hex character";
            return false;
        }
        out[i / 2] = static_cast<uint8_t>((h << 4) | l);
    }
    return true;
}

} // namespace shivrai::evm
