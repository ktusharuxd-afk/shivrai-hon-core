#include "test_framework.h"
#include "shivrai/monetary_policy.h"

using namespace shivrai;

TEST(policy_defaults) {
    const auto& p = GlobalMonetaryPolicy();
    ASSERT_EQ(p.initial_block_reward_hon, 2000u);
    ASSERT_EQ(p.target_block_time_seconds, 28800u);
    ASSERT_EQ(p.halving_interval_blocks, 210000u);
    ASSERT_FALSE(p.fixed_supply);
}

TEST(subsidy_genesis) {
    ASSERT_EQ(BlockSubsidyHON(0), 2000u);
    ASSERT_EQ(BlockSubsidyHON(1), 2000u);
    ASSERT_EQ(BlockSubsidyHON(209999), 2000u);
}

TEST(subsidy_first_halving) {
    ASSERT_EQ(BlockSubsidyHON(210000), 1000u);
    ASSERT_EQ(BlockSubsidyHON(419999), 1000u);
}

TEST(subsidy_second_halving) {
    ASSERT_EQ(BlockSubsidyHON(420000), 500u);
    ASSERT_EQ(BlockSubsidyHON(629999), 500u);
}

TEST(subsidy_eventually_zero) {
    ASSERT_EQ(BlockSubsidyHON(210000ULL * 63), 0u);
    ASSERT_EQ(BlockSubsidyHON(210000ULL * 64), 0u);
}

TEST(halving_count) {
    ASSERT_EQ(HalvingCount(0), 0u);
    ASSERT_EQ(HalvingCount(209999), 0u);
    ASSERT_EQ(HalvingCount(210000), 1u);
    ASSERT_EQ(HalvingCount(419999), 1u);
    ASSERT_EQ(HalvingCount(420000), 2u);
}

TEST(halving_era) {
    ASSERT_EQ(HalvingEra(0), 0u);
    ASSERT_EQ(HalvingEra(500000), 2u);
}

TEST(blocks_until_halving) {
    ASSERT_EQ(BlocksUntilNextHalving(0), 210000u);
    ASSERT_EQ(BlocksUntilNextHalving(100000), 110000u);
    ASSERT_EQ(BlocksUntilNextHalving(210000), 210000u);
}

TEST(max_supply) {
    // 2000 * 210000 * 2 = 840,000,000
    ASSERT_EQ(MaxSupplyHON(), 840000000u);
}

TEST(total_mined_at_genesis) {
    ASSERT_EQ(TotalMinedAtHeight(0), 0u);
}

TEST(total_mined_at_100k) {
    // 100000 blocks × 2000 HON = 200,000,000
    ASSERT_EQ(TotalMinedAtHeight(100000), 200000000u);
}

TEST(total_mined_at_210k) {
    // 210000 × 2000 = 420,000,000 (era 0 complete)
    ASSERT_EQ(TotalMinedAtHeight(210000), 420000000u);
}

TEST(remaining_supply) {
    ASSERT_EQ(RemainingSupplyAtHeight(0), 840000000u);
    ASSERT_EQ(RemainingSupplyAtHeight(210000), 420000000u);
}

TEST(mined_percentage_bps) {
    ASSERT_EQ(MinedPercentageBPS(0), 0u);
    // 100k blocks mined 200M of 840M = 23.8095% ≈ 2380 bps
    ASSERT_EQ(MinedPercentageBPS(100000), 2380u);
}

TEST(validate_policy_defaults) {
    std::string err;
    ASSERT_TRUE(ValidatePolicy(GlobalMonetaryPolicy(), err));
    ASSERT_TRUE(err.empty());
}

TEST(validate_policy_invalid) {
    MonetaryPolicy p{0, 28800, 210000, false};
    std::string err;
    ASSERT_FALSE(ValidatePolicy(p, err));
    ASSERT_FALSE(err.empty());
}

TEST(describe_policy) {
    std::string desc = DescribeMonetaryPolicy();
    ASSERT_FALSE(desc.empty());
    // Should mention HON
    ASSERT_TRUE(desc.find("HON") != std::string::npos);
    ASSERT_TRUE(desc.find("2000") != std::string::npos);
}

int main() {
    return shivrai::test::TestRunner::instance().run();
}
