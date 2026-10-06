// Copyright (c) 2026-present The SHIVRAI HON Core developers
// Distributed under the MIT software license.
#ifndef SHIVRAI_HON_MONETARY_POLICY_H
#define SHIVRAI_HON_MONETARY_POLICY_H
#include <cstdint>
namespace shivrai {
struct MonetaryPolicy {
    uint64_t initial_block_reward_hon;
    uint32_t target_block_time_seconds;
    uint64_t halving_interval_blocks;
    bool fixed_supply;
};
const MonetaryPolicy& GlobalMonetaryPolicy();
uint64_t BlockSubsidyHON(uint64_t height);
} // namespace shivrai
#endif
