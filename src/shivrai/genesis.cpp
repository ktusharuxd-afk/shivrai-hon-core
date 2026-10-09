// Copyright (c) 2026-present The SHIVRAI HON Core developers
// Distributed under the MIT software license.
#include <shivrai/genesis.h>
#include <sstream>
#include <iomanip>

namespace shivrai {

const GenesisSpec& GlobalGenesisSpec()
{
    static const GenesisSpec spec{
        "shivrai-mainnet-v2",
        "SHIVRAI HON GLOBAL GENESIS | Decentralized payments, finance, gaming and open blockchain infrastructure",
        0,
        0,
        0x1d00ffff,
        1,
        2000,
        false,
    };
    return spec;
}

bool ValidateGenesisSpec(const GenesisSpec& spec, std::string& error)
{
    if (spec.chain_name.empty()) {
        error = "chain_name must not be empty";
        return false;
    }
    if (spec.message.empty()) {
        error = "message must not be empty";
        return false;
    }
    if (spec.bits == 0) {
        error = "bits (difficulty) must be non-zero";
        return false;
    }
    if (spec.version == 0) {
        error = "version must be non-zero";
        return false;
    }
    if (spec.initial_reward_hon == 0) {
        error = "initial_reward_hon must be non-zero";
        return false;
    }
    if (spec.frozen && spec.timestamp == 0) {
        error = "frozen genesis must have non-zero timestamp";
        return false;
    }
    if (spec.frozen && spec.nonce == 0) {
        error = "frozen genesis must have non-zero nonce";
        return false;
    }
    return true;
}

bool IsGenesisFrozen()
{
    return GlobalGenesisSpec().frozen;
}

// FNV-1a 64-bit hash — placeholder for genesis ID.
// NOTE: Production must use double-SHA256 of the serialized block header.
static uint64_t FNV1a(const std::string& data)
{
    uint64_t hash = 1469598103934665603ULL;
    for (unsigned char c : data) {
        hash ^= c;
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::string GenesisHashHex()
{
    const auto& spec = GlobalGenesisSpec();
    // Placeholder: hash of chain_name + message.
    std::string seed = spec.chain_name + "|" + spec.message;
    uint64_t h = FNV1a(seed);
    std::ostringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << h;
    return ss.str();
}

bool MatchesGenesisHash(const std::string& hash_hex)
{
    return hash_hex == GenesisHashHex();
}

std::string DescribeGenesis()
{
    const auto& spec = GlobalGenesisSpec();
    std::ostringstream ss;
    ss << "SHIVRAI HON Genesis:\n"
       << "  Chain: " << spec.chain_name << "\n"
       << "  Message: " << spec.message << "\n"
       << "  Timestamp: " << spec.timestamp << "\n"
       << "  Nonce: " << spec.nonce << "\n"
       << "  Bits: 0x" << std::hex << spec.bits << std::dec << "\n"
       << "  Version: " << spec.version << "\n"
       << "  Initial reward: " << spec.initial_reward_hon << " HON\n"
       << "  Frozen: " << (spec.frozen ? "yes" : "no") << "\n"
       << "  Genesis ID: " << GenesisHashHex();
    return ss.str();
}

} // namespace shivrai
