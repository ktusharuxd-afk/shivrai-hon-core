// Copyright (c) 2026-present The SHIVRAI HON Core developers
// Distributed under the MIT software license.
#ifndef SHIVRAI_HON_MONETARY_POLICY_H
#define SHIVRAI_HON_MONETARY_POLICY_H
#include <cstdint>
#include <string>

namespace shivrai {

// HON supply schedule.
// Design target: 4-year halving at 8-hour blocks, matching Bitcoin's 4-year cadence.
struct MonetaryPolicy {
    uint64_t initial_block_reward_hon;    // 2000 HON
    uint32_t target_block_time_seconds;   // 28800 sec = 8 hours
    uint64_t halving_interval_blocks;     // 210000 blocks
    bool fixed_supply;
};

// Global policy singleton (read-only).
const MonetaryPolicy& GlobalMonetaryPolicy();

// Core subsidy function. Returns HON reward at given height.
uint64_t BlockSubsidyHON(uint64_t height);

// Extended API:

// Number of halvings that have occurred at the given height.
uint64_t HalvingCount(uint64_t height);

// Which "era" (epoch) we're in: 0=initial, 1=first halving, etc.
uint64_t HalvingEra(uint64_t height);

// Blocks until next halving. Returns 0 if halvings exhausted.
uint64_t BlocksUntilNextHalving(uint64_t height);

// Total HON mined from genesis to (and including) this height.
uint64_t TotalMinedAtHeight(uint64_t height);

// Maximum possible supply of HON (theoretical cap).
uint64_t MaxSupplyHON();

// Remaining unmined HON at height.
uint64_t RemainingSupplyAtHeight(uint64_t height);

// Percentage of total supply mined at height (0-10000 = 0.00%-100.00%).
uint32_t MinedPercentageBPS(uint64_t height);

// Validate policy is internally consistent.
bool ValidatePolicy(const MonetaryPolicy& p, std::string& error);

// Human-readable summary (for logging/RPC).
std::string DescribeMonetaryPolicy();

} // namespace shivrai
#endif
