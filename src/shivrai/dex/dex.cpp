#include "dex.h"
#include <algorithm>
#include <cmath>
#include <ctime>

namespace shivrai::dex {

Dex::Dex() = default;

std::pair<std::string, std::string> Dex::OrderTokens(const std::string& a,
                                                     const std::string& b) {
    return (a < b) ? std::make_pair(a, b) : std::make_pair(b, a);
}

uint64_t Dex::Sqrt(uint64_t x) {
    if (x == 0) return 0;
    uint64_t r = (uint64_t)std::sqrt((double)x);
    // integer correction
    while (r > 0 && r * r > x) --r;
    while ((r + 1) * (r + 1) <= x) ++r;
    return r;
}

// ---- Pool lifecycle ----
std::optional<PoolId> Dex::CreatePool(const std::string& token_a,
                                      const std::string& token_b,
                                      uint32_t fee_bps) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (token_a.empty() || token_b.empty() || token_a == token_b) return std::nullopt;
    if (fee_bps < MIN_FEE_BPS || fee_bps > MAX_FEE_BPS) return std::nullopt;

    auto [ta, tb] = OrderTokens(token_a, token_b);
    auto key = std::make_pair(ta, tb);
    if (pool_index_.count(key)) return std::nullopt;

    PoolId id = next_pool_id_++;
    Pool p;
    p.id = id;
    p.token_a = ta;
    p.token_b = tb;
    p.fee_bps = fee_bps;
    p.created_at = (int64_t)std::time(nullptr);
    pools_[id] = p;
    pool_index_[key] = id;
    return id;
}

std::optional<PoolId> Dex::FindPool(const std::string& token_a,
                                    const std::string& token_b) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (token_a.empty() || token_b.empty() || token_a == token_b) return std::nullopt;
    auto key = OrderTokens(token_a, token_b);
    auto it = pool_index_.find(key);
    if (it == pool_index_.end()) return std::nullopt;
    return it->second;
}

std::optional<Pool> Dex::GetPool(PoolId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = pools_.find(id);
    if (it == pools_.end()) return std::nullopt;
    return it->second;
}

std::vector<PoolId> Dex::AllPools() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<PoolId> out;
    for (auto& [k, v] : pools_) out.push_back(k);
    return out;
}

size_t Dex::PoolCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return pools_.size();
}

// ---- Liquidity ----
std::optional<LiquidityResult> Dex::AddLiquidity(PoolId id,
                                                 uint64_t amount_a,
                                                 uint64_t amount_b,
                                                 uint64_t /*max_slippage_bps*/) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = pools_.find(id);
    if (it == pools_.end()) return std::nullopt;
    if (amount_a == 0 || amount_b == 0) return std::nullopt;

    Pool& p = it->second;
    LiquidityResult r;

    if (p.total_lp_shares == 0) {
        // Initial liquidity: LP = sqrt(a*b)
        uint64_t shares = Sqrt(amount_a) * Sqrt(amount_b);
        // Safer: sqrt(a*b) via 128-bit
        __uint128_t prod = (__uint128_t)amount_a * amount_b;
        shares = Sqrt((uint64_t)std::min<__uint128_t>(prod, UINT64_MAX));
        if (shares < MIN_LIQUIDITY) return std::nullopt;
        p.reserve_a += amount_a;
        p.reserve_b += amount_b;
        p.total_lp_shares = shares;
        lp_balances_[id]["__initial__"] += shares;
        r.lp_shares_minted = shares;
        r.amount_a_added = amount_a;
        r.amount_b_added = amount_b;
        return r;
    }

    // Proportional add: shares based on the smaller ratio
    __uint128_t sa = (__uint128_t)amount_a * p.total_lp_shares / p.reserve_a;
    __uint128_t sb = (__uint128_t)amount_b * p.total_lp_shares / p.reserve_b;
    uint64_t shares = (uint64_t)std::min(sa, sb);
    if (shares == 0) return std::nullopt;

    // Scale amounts to match shares
    uint64_t used_a = (uint64_t)((__uint128_t)shares * p.reserve_a / p.total_lp_shares);
    uint64_t used_b = (uint64_t)((__uint128_t)shares * p.reserve_b / p.total_lp_shares);
    if (used_a == 0 || used_b == 0) return std::nullopt;

    p.reserve_a += used_a;
    p.reserve_b += used_b;
    p.total_lp_shares += shares;
    lp_balances_[id]["__initial__"] += shares;
    r.lp_shares_minted = shares;
    r.amount_a_added = used_a;
    r.amount_b_added = used_b;
    return r;
}

std::optional<WithdrawResult> Dex::RemoveLiquidity(PoolId id,
                                                   uint64_t lp_shares,
                                                   uint64_t min_a,
                                                   uint64_t min_b) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = pools_.find(id);
    if (it == pools_.end()) return std::nullopt;
    Pool& p = it->second;
    if (lp_shares == 0 || lp_shares > p.total_lp_shares) return std::nullopt;

    uint64_t out_a = (uint64_t)((__uint128_t)lp_shares * p.reserve_a / p.total_lp_shares);
    uint64_t out_b = (uint64_t)((__uint128_t)lp_shares * p.reserve_b / p.total_lp_shares);
    if (out_a < min_a || out_b < min_b) return std::nullopt;

    p.reserve_a -= out_a;
    p.reserve_b -= out_b;
    p.total_lp_shares -= lp_shares;

    // Burn from initial sink
    auto& bal = lp_balances_[id]["__initial__"];
    bal = (bal >= lp_shares) ? (bal - lp_shares) : 0;

    WithdrawResult r;
    r.amount_a_returned = out_a;
    r.amount_b_returned = out_b;
    r.lp_shares_burned = lp_shares;
    return r;
}

// ---- Quote ----
std::optional<uint64_t> Dex::QuoteOut(PoolId id, bool token_in, uint64_t amount_in) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = pools_.find(id);
    if (it == pools_.end() || amount_in == 0) return std::nullopt;
    const Pool& p = it->second;
    uint64_t rin  = token_in ? p.reserve_a : p.reserve_b;
    uint64_t rout = token_in ? p.reserve_b : p.reserve_a;
    if (rin == 0 || rout == 0) return std::nullopt;

    uint64_t fee = (uint64_t)((__uint128_t)amount_in * p.fee_bps / BPS);
    uint64_t net_in = amount_in - fee;
    if (net_in == 0) return std::nullopt;

    // Constant-product: out = rout * net_in / (rin + net_in)
    __uint128_t num = (__uint128_t)rout * net_in;
    __uint128_t den = (__uint128_t)rin + net_in;
    uint64_t out = (uint64_t)(num / den);
    if (out == 0 || out >= rout) return std::nullopt;
    return out;
}

// ---- Swap ----
std::optional<SwapResult> Dex::Swap(PoolId id,
                                    bool token_in,
                                    uint64_t amount_in,
                                    uint64_t min_amount_out) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = pools_.find(id);
    if (it == pools_.end() || amount_in == 0) return std::nullopt;
    Pool& p = it->second;

    uint64_t& rin  = token_in ? p.reserve_a : p.reserve_b;
    uint64_t& rout = token_in ? p.reserve_b : p.reserve_a;
    if (rin == 0 || rout == 0) return std::nullopt;

    uint64_t fee = (uint64_t)((__uint128_t)amount_in * p.fee_bps / BPS);
    uint64_t net_in = amount_in - fee;
    if (net_in == 0) return std::nullopt;

    __uint128_t num = (__uint128_t)rout * net_in;
    __uint128_t den = (__uint128_t)rin + net_in;
    uint64_t out = (uint64_t)(num / den);
    if (out == 0 || out >= rout) return std::nullopt;
    if (out < min_amount_out) return std::nullopt;

    rin  += amount_in;
    rout -= out;

    if (token_in) p.cumulative_volume_a += amount_in;
    else          p.cumulative_volume_b += amount_in;

    SwapResult r;
    r.amount_in = amount_in;
    r.amount_out = out;
    r.fee_paid = fee;
    r.new_reserve_in = rin;
    r.new_reserve_out = rout;
    return r;
}

// ---- LP balances ----
uint64_t Dex::LpBalance(PoolId id, const std::string& holder) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = lp_balances_.find(id);
    if (it == lp_balances_.end()) return 0;
    auto hit = it->second.find(holder);
    return hit == it->second.end() ? 0 : hit->second;
}

bool Dex::TransferLp(PoolId id,
                     const std::string& from,
                     const std::string& to,
                     uint64_t shares) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (pools_.find(id) == pools_.end()) return false;
    if (from.empty() || to.empty() || from == to) return false;
    if (shares == 0) return false;

    auto& m = lp_balances_[id];
    if (m[from] < shares) return false;
    m[from] -= shares;
    m[to] += shares;
    return true;
}

// ---- Price ----
std::optional<double> Dex::SpotPrice(PoolId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = pools_.find(id);
    if (it == pools_.end()) return std::nullopt;
    if (it->second.reserve_a == 0) return std::nullopt;
    return (double)it->second.reserve_b / (double)it->second.reserve_a;
}

std::optional<double> Dex::InverseSpotPrice(PoolId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = pools_.find(id);
    if (it == pools_.end()) return std::nullopt;
    if (it->second.reserve_b == 0) return std::nullopt;
    return (double)it->second.reserve_a / (double)it->second.reserve_b;
}

// ---- Config ----
bool Dex::SetFeeBps(PoolId id, uint32_t fee_bps) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = pools_.find(id);
    if (it == pools_.end()) return false;
    if (fee_bps < MIN_FEE_BPS || fee_bps > MAX_FEE_BPS) return false;
    it->second.fee_bps = fee_bps;
    return true;
}

uint32_t Dex::FeeBps(PoolId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = pools_.find(id);
    return it == pools_.end() ? 0 : it->second.fee_bps;
}

}