// Copyright (c) 2026-present The SHIVRAI HON Core developers
// Distributed under the MIT software license.
#include <shivrai/monetary_policy.h>

namespace shivrai {
const MonetaryPolicy& GlobalMonetaryPolicy()
{
    static const MonetaryPolicy policy{2000, 300, 210000, false};
    return policy;
}

uint64_t BlockSubsidyHON(uint64_t height)
{
    const auto& p = GlobalMonetaryPolicy();
    const uint64_t halvings = height / p.halving_interval_blocks;
    if (halvings >= 63) return 0;
    return p.initial_block_reward_hon >> halvings;
}
} // namespace shivrai
