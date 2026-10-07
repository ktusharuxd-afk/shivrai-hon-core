#ifndef SHIVRAI_HON_AMM_H
#define SHIVRAI_HON_AMM_H
#include <cstdint>
#include <stdexcept>
namespace shivrai::defi {

struct Quote { uint64_t amount_out; uint64_t fee; };

struct LiquidityResult { uint64_t lp_tokens; };

class ConstantProductAMM {
public:
    ConstantProductAMM(uint64_t reserve_a, uint64_t reserve_b, uint32_t fee_bps = 30);

    // Swap
    Quote quote_a_for_b(uint64_t amount_in) const;
    Quote quote_b_for_a(uint64_t amount_in) const;
    bool swap_a_for_b(uint64_t amount_in, uint64_t min_out, uint64_t& out);
    bool swap_b_for_a(uint64_t amount_in, uint64_t min_out, uint64_t& out);

    // Liquidity
    LiquidityResult add_liquidity(uint64_t amount_a, uint64_t amount_b);
    bool remove_liquidity(uint64_t lp_tokens, uint64_t& out_a, uint64_t& out_b);

    // View
    uint64_t reserve_a() const { return a_; }
    uint64_t reserve_b() const { return b_; }
    uint64_t total_lp() const { return lp_supply_; }
    double spot_price_a_in_b() const;

private:
    uint64_t a_, b_, lp_supply_;
    uint32_t fee_bps_;
    Quote _quote(uint64_t in, uint64_t reserve_in, uint64_t reserve_out) const;
};

} // namespace shivrai::defi
#endif
