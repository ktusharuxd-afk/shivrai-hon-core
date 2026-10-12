#ifndef SHIVRAI_HON_DEX_H
#define SHIVRAI_HON_DEX_H

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace shivrai::dex {

using PoolId = uint64_t;

static constexpr uint32_t BPS = 10000;
static constexpr uint32_t MIN_FEE_BPS = 1;        // 0.01%
static constexpr uint32_t MAX_FEE_BPS = 1000;     // 10%
static constexpr uint32_t DEFAULT_FEE_BPS = 30;   // 0.30%
static constexpr uint64_t MIN_LIQUIDITY = 1000;

struct Pool {
    PoolId      id{0};
    std::string token_a;      // alphabetically-first token
    std::string token_b;
    uint64_t    reserve_a{0};
    uint64_t    reserve_b{0};
    uint64_t    total_lp_shares{0};
    uint32_t    fee_bps{DEFAULT_FEE_BPS};
    int64_t     created_at{0};
    uint64_t    cumulative_volume_a{0};
    uint64_t    cumulative_volume_b{0};
};

struct SwapResult {
    uint64_t amount_in{0};
    uint64_t amount_out{0};
    uint64_t fee_paid{0};
    uint64_t new_reserve_in{0};
    uint64_t new_reserve_out{0};
};

struct LiquidityResult {
    uint64_t lp_shares_minted{0};
    uint64_t amount_a_added{0};
    uint64_t amount_b_added{0};
};

struct WithdrawResult {
    uint64_t amount_a_returned{0};
    uint64_t amount_b_returned{0};
    uint64_t lp_shares_burned{0};
};

class Dex {
public:
    Dex();

    // ---- Pool lifecycle ----
    std::optional<PoolId> CreatePool(const std::string& token_a,
                                     const std::string& token_b,
                                     uint32_t fee_bps);
    std::optional<PoolId> FindPool(const std::string& token_a,
                                   const std::string& token_b) const;
    std::optional<Pool> GetPool(PoolId id) const;
    std::vector<PoolId> AllPools() const;
    size_t PoolCount() const;

    // ---- Liquidity ----
    std::optional<LiquidityResult> AddLiquidity(PoolId id,
                                                uint64_t amount_a,
                                                uint64_t amount_b,
                                                uint64_t max_slippage_bps = 500);
    std::optional<WithdrawResult>  RemoveLiquidity(PoolId id,
                                                   uint64_t lp_shares,
                                                   uint64_t min_a = 0,
                                                   uint64_t min_b = 0);

    // ---- Swap ----
    // token_in = true → swap token_a in for token_b out
    // token_in = false → swap token_b in for token_a out
    std::optional<SwapResult> Swap(PoolId id,
                                   bool token_in,
                                   uint64_t amount_in,
                                   uint64_t min_amount_out = 0);

    // ---- Quote (view-only, no state change) ----
    std::optional<uint64_t> QuoteOut(PoolId id, bool token_in, uint64_t amount_in) const;

    // ---- LP balances ----
    uint64_t LpBalance(PoolId id, const std::string& holder) const;
    bool     TransferLp(PoolId id,
                        const std::string& from,
                        const std::string& to,
                        uint64_t shares);

    // ---- Price / TWAP-lite ----
    std::optional<double> SpotPrice(PoolId id) const;         // token_b per token_a
    std::optional<double> InverseSpotPrice(PoolId id) const;  // token_a per token_b

    // ---- Config ----
    bool SetFeeBps(PoolId id, uint32_t fee_bps);
    uint32_t FeeBps(PoolId id) const;

private:
    static std::pair<std::string, std::string> OrderTokens(const std::string& a,
                                                            const std::string& b);
    static uint64_t Sqrt(uint64_t x);

    mutable std::recursive_mutex cs_;
    std::map<PoolId, Pool> pools_;
    std::map<std::pair<std::string, std::string>, PoolId> pool_index_;
    std::map<PoolId, std::map<std::string, uint64_t>> lp_balances_;

    PoolId next_pool_id_{1};
};

}
#endif