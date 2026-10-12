#include "amm.h"
#include <stdexcept>
#include <cmath>
#include <limits>

namespace shivrai::defi {

ConstantProductAMM::ConstantProductAMM(uint64_t a, uint64_t b, uint32_t f)
    : a_(a), b_(b), lp_supply_(0), fee_bps_(f) {
    if (!a_ || !b_ || f >= 10000)
        throw std::invalid_argument("invalid pool params");
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
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return _quote(in, a_, b_);
}

Quote ConstantProductAMM::quote_b_for_a(uint64_t in) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return _quote(in, b_, a_);
}

bool ConstantProductAMM::swap_a_for_b(uint64_t in, uint64_t min_out, uint64_t& out) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto q = _quote(in, a_, b_);
    if (q.amount_out < min_out || q.amount_out >= b_) return false;
    a_ += in;
    b_ -= q.amount_out;
    out = q.amount_out;
    return true;
}

bool ConstantProductAMM::swap_b_for_a(uint64_t in, uint64_t min_out, uint64_t& out) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto q = _quote(in, b_, a_);
    if (q.amount_out < min_out || q.amount_out >= a_) return false;
    b_ += in;
    a_ -= q.amount_out;
    out = q.amount_out;
    return true;
}

LiquidityResult ConstantProductAMM::add_liquidity(uint64_t amount_a, uint64_t amount_b) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
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
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!lp_tokens || lp_tokens > lp_supply_) return false;
    out_a = (uint64_t)((__uint128_t)lp_tokens * a_ / lp_supply_);
    out_b = (uint64_t)((__uint128_t)lp_tokens * b_ / lp_supply_);
    a_ -= out_a;
    b_ -= out_b;
    lp_supply_ -= lp_tokens;
    return true;
}

uint64_t ConstantProductAMM::reserve_a() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return a_;
}

uint64_t ConstantProductAMM::reserve_b() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return b_;
}

uint64_t ConstantProductAMM::total_lp() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return lp_supply_;
}

double ConstantProductAMM::spot_price_a_in_b() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!a_) return 0.0;
    return (double)b_ / (double)a_;
}

double ConstantProductAMM::spot_price_b_in_a() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!b_) return 0.0;
    return (double)a_ / (double)b_;
}

double ConstantProductAMM::k_value() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return (double)a_ * (double)b_;
}

bool ConstantProductAMM::has_liquidity() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return a_ > 0 && b_ > 0;
}

uint32_t ConstantProductAMM::price_impact_bps(uint64_t amount_in, bool a_to_b) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!amount_in) return 0;
    
    uint64_t res_in = a_to_b ? a_ : b_;
    uint64_t res_out = a_to_b ? b_ : a_;
    if (!res_in || !res_out) return 10000;
    
    auto q = _quote(amount_in, res_in, res_out);
    if (!q.amount_out) return 10000;
    
    // Spot price (no slippage): res_out / res_in
    // Execution price: q.amount_out / amount_in
    // Impact = 1 - (execution / spot)
    double spot = (double)res_out / (double)res_in;
    double exec = (double)q.amount_out / (double)amount_in;
    if (exec >= spot) return 0;
    double impact = (spot - exec) / spot;
    if (impact > 1.0) return 10000;
    return (uint32_t)(impact * 10000.0);
}

} // namespace shivrai::defi
