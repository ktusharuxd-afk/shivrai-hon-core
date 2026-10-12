#ifndef SHIVRAI_HON_AMM_H
#define SHIVRAI_HON_AMM_H

#include <cstdint>
#include <mutex>
#include <stdexcept>

namespace shivrai::defi {

struct Quote {
    uint64_t amount_out;
    uint64_t fee;
};

struct LiquidityResult {
    uint64_t lp_tokens;
};

class ConstantProductAMM {
public:
    ConstantProductAMM(uint64_t reserve_a, uint64_t reserve_b,
                       uint32_t fee_bps = 30);

    // ─── Swap ───
    Quote quote_a_for_b(uint64_t amount_in) const;
    Quote quote_b_for_a(uint64_t amount_in) const;
    bool swap_a_for_b(uint64_t amount_in, uint64_t min_out, uint64_t& out);
    bool swap_b_for_a(uint64_t amount_in, uint64_t min_out, uint64_t& out);

    // ─── Liquidity ───
    LiquidityResult add_liquidity(uint64_t amount_a, uint64_t amount_b);
    bool remove_liquidity(uint64_t lp_tokens, uint64_t& out_a, uint64_t& out_b);

    // ─── View ───
    uint64_t reserve_a() const;
    uint64_t reserve_b() const;
    uint64_t total_lp() const;
    uint32_t fee_bps() const { return fee_bps_; }

    double spot_price_a_in_b() const;
    double spot_price_b_in_a() const;

    // Product invariant k = a * b (as uint128 fit via double).
    double k_value() const;

    // Price impact in basis points for a hypothetical swap.
    // 0 = no impact, 10000 = 100% impact (unrealistic).
    uint32_t price_impact_bps(uint64_t amount_in, bool a_to_b) const;

    // Check if pool has usable liquidity.
    bool has_liquidity() const;

private:
    uint64_t a_, b_, lp_supply_;
    uint32_t fee_bps_;
    mutable std::recursive_mutex mutex_;

    Quote _quote(uint64_t in, uint64_t reserve_in, uint64_t reserve_out) const;
};

} // namespace shivrai::defi
#endif
