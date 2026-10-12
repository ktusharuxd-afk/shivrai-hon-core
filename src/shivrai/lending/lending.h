#ifndef SHIVRAI_HON_LENDING_H
#define SHIVRAI_HON_LENDING_H

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace shivrai::lending {

static constexpr uint64_t WAD = 1'000'000'000'000'000'000ULL;
static constexpr uint32_t BPS = 10000;
static constexpr int64_t  SECONDS_PER_YEAR = 31536000;
static constexpr uint32_t DEFAULT_COLLATERAL_FACTOR_BPS = 7500;
static constexpr uint32_t DEFAULT_LIQ_THRESHOLD_BPS     = 8000;
static constexpr uint32_t DEFAULT_LIQ_PENALTY_BPS       = 500;
static constexpr uint32_t DEFAULT_RESERVE_FACTOR_BPS    = 1000;

struct MarketConfig {
    uint32_t collateral_factor_bps{DEFAULT_COLLATERAL_FACTOR_BPS};
    uint32_t liquidation_threshold_bps{DEFAULT_LIQ_THRESHOLD_BPS};
    uint32_t liquidation_penalty_bps{DEFAULT_LIQ_PENALTY_BPS};
    uint32_t reserve_factor_bps{DEFAULT_RESERVE_FACTOR_BPS};
    uint32_t base_rate_bps{0};
    uint32_t slope1_bps{400};
    uint32_t slope2_bps{6000};
    uint32_t optimal_utilization_bps{8000};
};

struct Market {
    std::string asset;
    MarketConfig cfg;
    uint64_t total_supply{0};
    uint64_t total_borrows{0};
    uint64_t reserves{0};
    uint64_t supply_index{WAD};
    uint64_t borrow_index{WAD};
    int64_t  last_accrual{0};
};

struct UserPosition {
    uint64_t supply_shares{0};
    uint64_t borrow_shares{0};
};

struct AccountInfo {
    uint64_t total_collateral_value{0};
    uint64_t total_debt_value{0};
    uint32_t health_factor_bps{0};
    bool     liquidatable{false};
};

class LendingPool {
public:
    LendingPool();

    bool CreateMarket(const std::string& asset, const MarketConfig& cfg = {});
    bool SetCollateralFactor(const std::string& asset, uint32_t bps);
    bool SetLiquidationThreshold(const std::string& asset, uint32_t bps);
    bool SetReserveFactor(const std::string& asset, uint32_t bps);
    bool SetInterestModel(const std::string& asset,
                          uint32_t base_bps, uint32_t slope1_bps,
                          uint32_t slope2_bps, uint32_t optimal_bps);
    bool MarketExists(const std::string& asset) const;
    std::optional<MarketConfig> Config(const std::string& asset) const;

    std::optional<uint64_t> Supply(const std::string& asset, const std::string& user, uint64_t amount);
    std::optional<uint64_t> Withdraw(const std::string& asset, const std::string& user, uint64_t shares);
    std::optional<uint64_t> Borrow(const std::string& asset, const std::string& user, uint64_t amount);
    std::optional<uint64_t> Repay(const std::string& asset, const std::string& user, uint64_t amount);

    bool Liquidate(const std::string& debt_asset,
                   const std::string& collateral_asset,
                   const std::string& borrower,
                   const std::string& liquidator,
                   uint64_t debt_amount,
                   uint64_t debt_price,
                   uint64_t collateral_price);

    void AccrueInterest(const std::string& asset);

    uint64_t TotalSupply(const std::string& asset) const;
    uint64_t TotalBorrows(const std::string& asset) const;
    uint64_t Reserves(const std::string& asset) const;
    uint64_t SupplyBalance(const std::string& asset, const std::string& user) const;
    uint64_t BorrowBalance(const std::string& asset, const std::string& user) const;
    uint32_t UtilizationBps(const std::string& asset) const;
    uint32_t BorrowRateBps(const std::string& asset) const;
    uint32_t SupplyRateBps(const std::string& asset) const;
    size_t   MarketCount() const;

    AccountInfo GetAccountInfo(const std::string& user,
                               const std::map<std::string, uint64_t>& prices) const;

private:
    uint32_t ComputeBorrowRate(const Market& m, uint32_t U) const;
    void     AccrueMarket(Market& m, int64_t now);

    mutable std::recursive_mutex cs_;
    std::map<std::string, Market> markets_;
    std::map<std::string, std::map<std::string, UserPosition>> positions_;
};

}
#endif