// Copyright (c) 2026-present The SHIVRAI HON Core developers
// Distributed under the MIT software license.
#include <shivrai/monetary_policy.h>
#include <sstream>
#include <limits>

namespace shivrai {

const MonetaryPolicy& GlobalMonetaryPolicy()
{
    static const MonetaryPolicy policy{2000, 28800, 210000, false};
    return policy;
}

uint64_t HalvingCount(uint64_t height)
{
    const auto& p = GlobalMonetaryPolicy();
    if (p.halving_interval_blocks == 0) return 0;
    return height / p.halving_interval_blocks;
}

uint64_t HalvingEra(uint64_t height)
{
    return HalvingCount(height);
}

uint64_t BlockSubsidyHON(uint64_t height)
{
    const auto& p = GlobalMonetaryPolicy();
    const uint64_t halvings = HalvingCount(height);
    if (halvings >= 63) return 0;
    return p.initial_block_reward_hon >> halvings;
}

uint64_t BlocksUntilNextHalving(uint64_t height)
{
    const auto& p = GlobalMonetaryPolicy();
    if (p.halving_interval_blocks == 0) return 0;
    const uint64_t next = (HalvingCount(height) + 1) * p.halving_interval_blocks;
    if (next <= height) return 0;
    return next - height;
}

uint64_t TotalMinedAtHeight(uint64_t height)
{
    const auto& p = GlobalMonetaryPolicy();
    if (p.halving_interval_blocks == 0) return 0;

    uint64_t total = 0;
    uint64_t remaining = height;
    uint64_t reward = p.initial_block_reward_hon;

    while (remaining > 0 && reward > 0) {
        const uint64_t chunk = (remaining < p.halving_interval_blocks)
                               ? remaining
                               : p.halving_interval_blocks;
        total += chunk * reward;
        remaining -= chunk;
        reward >>= 1;
    }
    return total;
}

uint64_t MaxSupplyHON()
{
    const auto& p = GlobalMonetaryPolicy();
    if (p.halving_interval_blocks == 0) return 0;
    return p.initial_block_reward_hon * p.halving_interval_blocks * 2;
}

uint64_t RemainingSupplyAtHeight(uint64_t height)
{
    const uint64_t max = MaxSupplyHON();
    const uint64_t mined = TotalMinedAtHeight(height);
    return (mined >= max) ? 0 : (max - mined);
}

uint32_t MinedPercentageBPS(uint64_t height)
{
    const uint64_t max = MaxSupplyHON();
    if (max == 0) return 0;
    const uint64_t mined = TotalMinedAtHeight(height);
    if (mined >= max) return 10000;
    const uint64_t bps = (mined > (std::numeric_limits<uint64_t>::max() / 10000))
                         ? 10000
                         : (mined * 10000 / max);
    return static_cast<uint32_t>(bps);
}

bool ValidatePolicy(const MonetaryPolicy& p, std::string& error)
{
    if (p.initial_block_reward_hon == 0) {
        error = "initial_block_reward_hon must be > 0";
        return false;
    }
    if (p.target_block_time_seconds == 0) {
        error = "target_block_time_seconds must be > 0";
        return false;
    }
    if (p.halving_interval_blocks == 0) {
        error = "halving_interval_blocks must be > 0";
        return false;
    }
    if (p.initial_block_reward_hon > (std::numeric_limits<uint64_t>::max() >> 1)) {
        error = "initial_block_reward_hon too large";
        return false;
    }
    return true;
}

std::string DescribeMonetaryPolicy()
{
    const auto& p = GlobalMonetaryPolicy();
    std::ostringstream ss;
    ss << "SHIVRAI HON Monetary Policy:\n"
       << "  Initial block reward: " << p.initial_block_reward_hon << " HON\n"
       << "  Target block time: " << p.target_block_time_seconds << " seconds\n"
       << "  Halving interval: " << p.halving_interval_blocks << " blocks\n"
       << "  Max supply: " << MaxSupplyHON() << " HON\n"
       << "  Fixed supply: " << (p.fixed_supply ? "yes" : "no") << "\n"
       << "  Years per era: ~"
       << (p.halving_interval_blocks * p.target_block_time_seconds / 31536000ULL)
       << " years";
    return ss.str();
}

} // namespace shivrai
