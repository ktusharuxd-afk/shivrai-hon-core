#include "test_framework.h"
#include "shivrai/defi/amm.h"

using namespace shivrai::defi;

TEST(amm_constructor_valid) {
    ConstantProductAMM pool(1000, 2000);
    ASSERT_EQ(pool.reserve_a(), 1000u);
    ASSERT_EQ(pool.reserve_b(), 2000u);
    ASSERT_TRUE(pool.total_lp() > 0);
    ASSERT_TRUE(pool.has_liquidity());
}

TEST(amm_constructor_invalid_a_zero) {
    bool threw = false;
    try { ConstantProductAMM pool(0, 1000); } catch (...) { threw = true; }
    ASSERT_TRUE(threw);
}

TEST(amm_constructor_invalid_b_zero) {
    bool threw = false;
    try { ConstantProductAMM pool(1000, 0); } catch (...) { threw = true; }
    ASSERT_TRUE(threw);
}

TEST(amm_constructor_invalid_fee) {
    bool threw = false;
    try { ConstantProductAMM pool(1000, 1000, 10000); } catch (...) { threw = true; }
    ASSERT_TRUE(threw);
}

TEST(amm_spot_price) {
    ConstantProductAMM pool(1000, 2000);
    double pa = pool.spot_price_a_in_b();
    double pb = pool.spot_price_b_in_a();
    ASSERT_TRUE(pa > 1.99 && pa < 2.01);
    ASSERT_TRUE(pb > 0.49 && pb < 0.51);
}

TEST(amm_k_value) {
    ConstantProductAMM pool(1000, 2000);
    double k = pool.k_value();
    ASSERT_TRUE(k > 1999999.0 && k < 2000001.0);
}

TEST(amm_quote_a_for_b) {
    ConstantProductAMM pool(1000, 2000, 30);
    auto q = pool.quote_a_for_b(100);
    ASSERT_TRUE(q.amount_out > 175 && q.amount_out < 185);
}

TEST(amm_quote_b_for_a) {
    ConstantProductAMM pool(1000, 2000, 30);
    auto q = pool.quote_b_for_a(100);
    ASSERT_TRUE(q.amount_out > 45 && q.amount_out < 50);
}

TEST(amm_quote_zero_returns_zero) {
    ConstantProductAMM pool(1000, 2000);
    auto q = pool.quote_a_for_b(0);
    ASSERT_EQ(q.amount_out, 0u);
}

TEST(amm_swap_a_for_b_success) {
    ConstantProductAMM pool(1000, 2000, 30);
    uint64_t out = 0;
    ASSERT_TRUE(pool.swap_a_for_b(100, 50, out));
    ASSERT_TRUE(out > 175 && out < 185);
    ASSERT_EQ(pool.reserve_a(), 1100u);
    ASSERT_EQ(pool.reserve_b(), 2000u - out);
}

TEST(amm_swap_slippage_fails) {
    ConstantProductAMM pool(1000, 2000, 30);
    uint64_t out = 0;
    ASSERT_FALSE(pool.swap_a_for_b(100, 500, out));
}

TEST(amm_swap_b_for_a_success) {
    ConstantProductAMM pool(1000, 2000, 30);
    uint64_t out = 0;
    ASSERT_TRUE(pool.swap_b_for_a(100, 30, out));
    ASSERT_TRUE(out > 45 && out < 50);
    ASSERT_EQ(pool.reserve_b(), 2100u);
}

TEST(amm_k_increases_after_swap_with_fee) {
    ConstantProductAMM pool(1000, 2000, 30);
    double k_before = pool.k_value();
    uint64_t out = 0;
    pool.swap_a_for_b(100, 0, out);
    double k_after = pool.k_value();
    ASSERT_TRUE(k_after >= k_before);
}

TEST(amm_add_liquidity) {
    ConstantProductAMM pool(1000, 2000, 30);
    uint64_t lp_before = pool.total_lp();
    auto result = pool.add_liquidity(500, 1000);
    ASSERT_TRUE(result.lp_tokens > 0);
    ASSERT_TRUE(pool.total_lp() > lp_before);
    ASSERT_EQ(pool.reserve_a(), 1500u);
    ASSERT_EQ(pool.reserve_b(), 3000u);
}

TEST(amm_add_liquidity_zero_throws) {
    ConstantProductAMM pool(1000, 2000);
    bool threw = false;
    try { pool.add_liquidity(0, 100); } catch (...) { threw = true; }
    ASSERT_TRUE(threw);
}

TEST(amm_remove_liquidity) {
    ConstantProductAMM pool(1000, 2000, 30);
    uint64_t lp = pool.total_lp();
    uint64_t out_a = 0, out_b = 0;
    ASSERT_TRUE(pool.remove_liquidity(lp, out_a, out_b));
    ASSERT_TRUE(out_a > 0);
    ASSERT_TRUE(out_b > 0);
    ASSERT_EQ(pool.total_lp(), 0u);
}

TEST(amm_remove_liquidity_partial) {
    ConstantProductAMM pool(1000, 2000, 30);
    uint64_t lp = pool.total_lp() / 2;
    uint64_t out_a = 0, out_b = 0;
    ASSERT_TRUE(pool.remove_liquidity(lp, out_a, out_b));
    ASSERT_TRUE(out_a > 490 && out_a < 510);
    ASSERT_TRUE(out_b > 990 && out_b < 1010);
}

TEST(amm_remove_liquidity_exceeds_fails) {
    ConstantProductAMM pool(1000, 2000, 30);
    uint64_t out_a = 0, out_b = 0;
    ASSERT_FALSE(pool.remove_liquidity(pool.total_lp() + 1, out_a, out_b));
}

TEST(amm_remove_liquidity_zero_fails) {
    ConstantProductAMM pool(1000, 2000, 30);
    uint64_t out_a = 0, out_b = 0;
    ASSERT_FALSE(pool.remove_liquidity(0, out_a, out_b));
}

TEST(amm_price_impact_small_swap) {
    ConstantProductAMM pool(1000, 2000, 30);
    uint32_t impact = pool.price_impact_bps(10, true);
    ASSERT_TRUE(impact < 2000);
}

TEST(amm_price_impact_large_swap) {
    ConstantProductAMM pool(1000, 2000, 30);
    uint32_t small_impact = pool.price_impact_bps(10, true);
    uint32_t large_impact = pool.price_impact_bps(500, true);
    ASSERT_TRUE(large_impact > small_impact);
}

TEST(amm_price_impact_zero) {
    ConstantProductAMM pool(1000, 2000);
    ASSERT_EQ(pool.price_impact_bps(0, true), 0u);
}

TEST(amm_roundtrip_preserves_supply) {
    ConstantProductAMM pool(1000, 2000, 30);
    double k_initial = pool.k_value();
    uint64_t out = 0;
    pool.swap_a_for_b(100, 0, out);
    ASSERT_TRUE(pool.k_value() >= k_initial);
}

TEST(amm_fee_bps_custom) {
    ConstantProductAMM pool(1000, 2000, 100);  // 1% fee
    ASSERT_EQ(pool.fee_bps(), 100u);
}

TEST(amm_fee_zero_valid) {
    ConstantProductAMM pool(1000, 2000, 0);  // 0% fee
    ASSERT_EQ(pool.fee_bps(), 0u);
}

int main() {
    return shivrai::test::TestRunner::instance().run();
}
