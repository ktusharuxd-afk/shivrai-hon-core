#include "amm.h"
#include <stdexcept>
#include <cmath>

namespace shivrai::defi {

ConstantProductAMM::ConstantProductAMM(uint64_t a, uint64_t b, uint32_t f)
    : a_(a), b_(b), lp_supply_(0), fee_bps_(f) {
    if (!a_ || !b_ || f >= 10000)
        throw std::invalid_argument("invalid pool params");
    // Initial LP = sqrt(a * b)
    lp_supply_ = (uint64_t)std::sqrt((double)a_ * (double)b_);
}

Quote ConstantProductAMM::_quote(uint64_t in, uint64_t res_in, uint64_t res_out) const {
    if (!in) return {0, 0};
    const uint64_t fee = (in * fee_bps_) / 10000;
    const uint64_t net = in - fee;
    __uint128_t num = (__uint128_t)net * res_out;
    __uint128_t den = (__uint128_t)res_in + net;
    uint64_t out = (uint64_t)(num / den);
    return {out, fee};
}

Quote ConstantProductAMM::quote_a_for_b(uint64_t in) const {
    return _quote(in, a_, b_);
}

Quote ConstantProductAMM::quote_b_for_a(uint64_t in) const {
    return _quote(in, b_, a_);
}

bool ConstantProductAMM::swap_a_for_b(uint64_t in, uint64_t min_out, uint64_t& out) {
    auto q = _quote(in, a_, b_);
    if (q.amount_out < min_out || q.amount_out >= b_) return false;
    a_ += in;
    b_ -= q.amount_out;
    out = q.amount_out;
    return true;
}

bool ConstantProductAMM::swap_b_for_a(uint64_t in, uint64_t min_out, uint64_t& out) {
    auto q = _quote(in, b_, a_);
    if (q.amount_out < min_out || q.amount_out >= a_) return false;
    b_ += in;
    a_ -= q.amount_out;
    out = q.amount_out;
    return true;
}

LiquidityResult ConstantProductAMM::add_liquidity(uint64_t amount_a, uint64_t amount_b) {
    if (!amount_a || !amount_b) throw std::invalid_argument("zero liquidity");
    uint64_t lp;
    if (lp_supply_ == 0) {
        lp = (uint64_t)std::sqrt((double)amount_a * (double)amount_b);
    } else {
        uint64_t lp_a = (uint64_t)((__uint128_t)amount_a * lp_supply_ / a_);
        uint64_t lp_b = (uint64_t)((__uint128_t)amount_b * lp_supply_ / b_);
        lp = lp_a < lp_b ? lp_a : lp_b;
    }
    a_ += amount_a;
    b_ += amount_b;
    lp_supply_ += lp;
    return {lp};
}

bool ConstantProductAMM::remove_liquidity(uint64_t lp_tokens, uint64_t& out_a, uint64_t& out_b) {
    if (!lp_tokens || lp_tokens > lp_supply_) return false;
    out_a = (uint64_t)((__uint128_t)lp_tokens * a_ / lp_supply_);
    out_b = (uint64_t)((__uint128_t)lp_tokens * b_ / lp_supply_);
    a_ -= out_a;
    b_ -= out_b;
    lp_supply_ -= lp_tokens;
    return true;
}

double ConstantProductAMM::spot_price_a_in_b() const {
    if (!a_) return 0.0;
    return (double)b_ / (double)a_;
}

} // namespace shivrai::defi
