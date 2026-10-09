// Copyright (c) 2026-present The SHIVRAI HON Core developers
// Distributed under the MIT software license.
#ifndef SHIVRAI_HON_GENESIS_H
#define SHIVRAI_HON_GENESIS_H

#include <cstdint>
#include <string>

namespace shivrai {

// Genesis block specification.
// All values must be frozen at mainnet launch ceremony time and never changed.
struct GenesisSpec {
    std::string chain_name;
    std::string message;             // Embedded in coinbase
    uint64_t timestamp;              // Unix seconds
    uint64_t nonce;                  // PoW nonce
    uint32_t bits;                   // Difficulty target (compact form)
    uint32_t version;                // Block version
    uint64_t initial_reward_hon;     // Coinbase reward
    bool frozen;                     // Whether the spec is locked
};

// Global spec singleton (read-only).
const GenesisSpec& GlobalGenesisSpec();

// Validate spec is internally consistent.
bool ValidateGenesisSpec(const GenesisSpec& spec, std::string& error);

// Whether the genesis is frozen (launch ceremony complete).
bool IsGenesisFrozen();

// Canonical genesis hash (hex, big-endian display form).
// Placeholder: returns the double-SHA256 of the message field.
std::string GenesisHashHex();

// Check if a provided hash matches the canonical genesis hash.
bool MatchesGenesisHash(const std::string& hash_hex);

// Human-readable summary (for logging / RPC).
std::string DescribeGenesis();

} // namespace shivrai
#endif // SHIVRAI_HON_GENESIS_H
