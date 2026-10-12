#include "lending.h"
#include <algorithm>
#include <ctime>

namespace shivrai::lending {

LendingPool::LendingPool() = default;

bool LendingPool::CreateMarket(const std::string& asset, const MarketConfig& cfg) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (markets_.count(asset)) return false;
    Market m;
    m.asset = asset;
    m.cfg = cfg;
    m.last_accrual = (int64_t)std::time(nullptr);
    markets_[asset] = m;
    return true;
}

bool LendingPool::SetCollateralFactor(const std::string& asset, uint32_t bps) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = markets_.find(asset);
    if (it == markets_.end() || bps > BPS) return false;
    it->second.cfg.collateral_factor_bps = bps;
    return true;
}

bool LendingPool::SetLiquidationThreshold(const std::string& asset, uint32_t bps) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = markets_.find(asset);
    if (it == markets_.end() || bps > BPS) return false;
    it->second.cfg.liquidation_threshold_bps = bps;
    return true;
}

bool LendingPool::SetReserveFactor(const std::string& asset, uint32_t bps) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = markets_.find(asset);
    if (it == markets_.end() || bps > BPS) return false;
    it->second.cfg.reserve_factor_bps = bps;
    return true;
}

bool LendingPool::SetInterestModel(const std::string& asset,
                                   uint32_t base_bps, uint32_t slope1_bps,
                                   uint32_t slope2_bps, uint32_t optimal_bps) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = markets_.find(asset);
    if (it == markets_.end() || optimal_bps == 0 || optimal_bps > BPS) return false;
    it->second.cfg.base_rate_bps = base_bps;
    it->second.cfg.slope1_bps = slope1_bps;
    it->second.cfg.slope2_bps = slope2_bps;
    it->second.cfg.optimal_utilization_bps = optimal_bps;
    return true;
}

bool LendingPool::MarketExists(const std::string& asset) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return markets_.count(asset) > 0;
}

std::optional<MarketConfig> LendingPool::Config(const std::string& asset) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = markets_.find(asset);
    if (it == markets_.end()) return std::nullopt;
    return it->second.cfg;
}

uint32_t LendingPool::ComputeBorrowRate(const Market& m, uint32_t U) const {
    const auto& c = m.cfg;
    if (U <= c.optimal_utilization_bps) {
        if (c.optimal_utilization_bps == 0) return c.base_rate_bps;
        return c.base_rate_bps + (uint32_t)((uint64_t)c.slope1_bps * U / c.optimal_utilization_bps);
    }
    uint32_t excess = U - c.optimal_utilization_bps;
    uint32_t range = BPS - c.optimal_utilization_bps;
    if (range == 0) return c.base_rate_bps + c.slope1_bps;
    return c.base_rate_bps + c.slope1_bps + (uint32_t)((uint64_t)c.slope2_bps * excess / range);
}

void LendingPool::AccrueMarket(Market& m, int64_t now) {
    if (m.last_accrual == 0 || m.total_borrows == 0) {
        m.last_accrual = now;
        return;
    }
    int64_t dt = now - m.last_accrual;
    if (dt <= 0) { m.last_accrual = now; return; }

    uint64_t avail = m.total_supply > m.reserves ? m.total_supply - m.reserves : 0;
    if (avail == 0) { m.last_accrual = now; return; }
    uint32_t U = m.total_borrows >= avail ? BPS : (uint32_t)(m.total_borrows * BPS / avail);
    uint32_t br = ComputeBorrowRate(m, U);

    __uint128_t num = (__uint128_t)m.total_borrows * br * (uint64_t)dt;
    __uint128_t den = (__uint128_t)BPS * (uint64_t)SECONDS_PER_YEAR;
    uint64_t interest = (uint64_t)(num / den);
    if (interest == 0) { m.last_accrual = now; return; }

    uint64_t to_reserve = interest * m.cfg.reserve_factor_bps / BPS;
    uint64_t to_suppliers = interest - to_reserve;

    uint64_t old_borrows = m.total_borrows;
    uint64_t old_supply  = m.total_supply;

    m.total_borrows += interest;
    m.reserves += to_reserve;
    m.total_supply += to_suppliers;

    if (old_borrows > 0) {
        m.borrow_index = (uint64_t)((__uint128_t)m.borrow_index * m.total_borrows / old_borrows);
    }
    if (old_supply > 0) {
        m.supply_index = (uint64_t)((__uint128_t)m.supply_index * m.total_supply / old_supply);
    }

    m.last_accrual = now;
}

void LendingPool::AccrueInterest(const std::string& asset) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = markets_.find(asset);
    if (it == markets_.end()) return;
    AccrueMarket(it->second, (int64_t)std::time(nullptr));
}

std::optional<uint64_t> LendingPool::Supply(const std::string& asset, const std::string& user, uint64_t amount) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = markets_.find(asset);
    if (it == markets_.end() || amount == 0) return std::nullopt;
    AccrueMarket(it->second, (int64_t)std::time(nullptr));
    auto& m = it->second;
    uint64_t shares = (uint64_t)((__uint128_t)amount * WAD / m.supply_index);
    if (shares == 0) return std::nullopt;
    m.total_supply += amount;
    positions_[user][asset].supply_shares += shares;
    return shares;
}

std::optional<uint64_t> LendingPool::Withdraw(const std::string& asset, const std::string& user, uint64_t shares) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = markets_.find(asset);
    if (it == markets_.end() || shares == 0) return std::nullopt;
    AccrueMarket(it->second, (int64_t)std::time(nullptr));
    auto& m = it->second;
    auto pit = positions_.find(user);
    if (pit == positions_.end()) return std::nullopt;
    auto uit = pit->second.find(asset);
    if (uit == pit->second.end() || uit->second.supply_shares < shares) return std::nullopt;
    uint64_t amount = (uint64_t)((__uint128_t)shares * m.supply_index / WAD);
    uint64_t avail = m.total_supply > m.total_borrows ? m.total_supply - m.total_borrows : 0;
    if (amount > avail) return std::nullopt;
    uit->second.supply_shares -= shares;
    m.total_supply -= amount;
    return amount;
}

std::optional<uint64_t> LendingPool::Borrow(const std::string& asset, const std::string& user, uint64_t amount) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = markets_.find(asset);
    if (it == markets_.end() || amount == 0) return std::nullopt;
    AccrueMarket(it->second, (int64_t)std::time(nullptr));
    auto& m = it->second;
    uint64_t avail = m.total_supply > m.total_borrows ? m.total_supply - m.total_borrows : 0;
    if (amount > avail) return std::nullopt;
    uint64_t shares = (uint64_t)((__uint128_t)amount * WAD / m.borrow_index);
    if (shares == 0) return std::nullopt;
    m.total_borrows += amount;
    positions_[user][asset].borrow_shares += shares;
    return amount;
}

std::optional<uint64_t> LendingPool::Repay(const std::string& asset, const std::string& user, uint64_t amount) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = markets_.find(asset);
    if (it == markets_.end() || amount == 0) return std::nullopt;
    AccrueMarket(it->second, (int64_t)std::time(nullptr));
    auto& m = it->second;
    auto pit = positions_.find(user);
    if (pit == positions_.end()) return std::nullopt;
    auto uit = pit->second.find(asset);
    if (uit == pit->second.end() || uit->second.borrow_shares == 0) return std::nullopt;
    uint64_t owed = (uint64_t)((__uint128_t)uit->second.borrow_shares * m.borrow_index / WAD);
    uint64_t actual = std::min(amount, owed);
    if (actual == 0) return std::nullopt;
    uint64_t shares_to_burn = (uint64_t)((__uint128_t)actual * WAD / m.borrow_index);
    if (shares_to_burn > uit->second.borrow_shares) shares_to_burn = uit->second.borrow_shares;
    uit->second.borrow_shares -= shares_to_burn;
    m.total_borrows -= actual;
    return actual;
}

bool LendingPool::Liquidate(const std::string& debt_asset,
                            const std::string& collateral_asset,
                            const std::string& borrower,
                            const std::string& liquidator,
                            uint64_t debt_amount,
                            uint64_t debt_price,
                            uint64_t collateral_price) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto dit = markets_.find(debt_asset);
    auto cit = markets_.find(collateral_asset);
    if (dit == markets_.end() || cit == markets_.end()) return false;
    AccrueMarket(dit->second, (int64_t)std::time(nullptr));
    AccrueMarket(cit->second, (int64_t)std::time(nullptr));

    auto pit = positions_.find(borrower);
    if (pit == positions_.end()) return false;
    auto duit = pit->second.find(debt_asset);
    auto cuit = pit->second.find(collateral_asset);
    if (duit == pit->second.end() || cuit == pit->second.end()) return false;

    uint64_t owed = (uint64_t)((__uint128_t)duit->second.borrow_shares * dit->second.borrow_index / WAD);
    if (owed == 0) return false;
    uint64_t actual = std::min(debt_amount, owed);
    if (actual == 0) return false;

    uint64_t collateral_amount = (uint64_t)((__uint128_t)cuit->second.supply_shares * cit->second.supply_index / WAD);
    __uint128_t coll_val = (__uint128_t)collateral_amount * collateral_price;
    __uint128_t debt_val = (__uint128_t)owed * debt_price;
    if (debt_val == 0) return false;
    __uint128_t hf_bps = coll_val * cit->second.cfg.liquidation_threshold_bps / debt_val;
    if (hf_bps >= BPS) return false;

    __uint128_t debt_paid_value = (__uint128_t)actual * debt_price;
    __uint128_t seized_value = debt_paid_value * (BPS + cit->second.cfg.liquidation_penalty_bps) / BPS;
    uint64_t seized = collateral_price > 0 ? (uint64_t)(seized_value / collateral_price) : 0;
    if (seized > collateral_amount) seized = collateral_amount;

    uint64_t shares_to_burn = (uint64_t)((__uint128_t)actual * WAD / dit->second.borrow_index);
    if (shares_to_burn > duit->second.borrow_shares) shares_to_burn = duit->second.borrow_shares;
    duit->second.borrow_shares -= shares_to_burn;
    dit->second.total_borrows -= actual;

    uint64_t shares_seized = (uint64_t)((__uint128_t)seized * WAD / cit->second.supply_index);
    if (shares_seized > cuit->second.supply_shares) shares_seized = cuit->second.supply_shares;
    cuit->second.supply_shares -= shares_seized;
    positions_[liquidator][collateral_asset].supply_shares += shares_seized;

    return true;
}

uint64_t LendingPool::TotalSupply(const std::string& asset) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = markets_.find(asset);
    return it == markets_.end() ? 0 : it->second.total_supply;
}
uint64_t LendingPool::TotalBorrows(const std::string& asset) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = markets_.find(asset);
    return it == markets_.end() ? 0 : it->second.total_borrows;
}
uint64_t LendingPool::Reserves(const std::string& asset) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = markets_.find(asset);
    return it == markets_.end() ? 0 : it->second.reserves;
}
uint64_t LendingPool::SupplyBalance(const std::string& asset, const std::string& user) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto mit = markets_.find(asset);
    if (mit == markets_.end()) return 0;
    auto pit = positions_.find(user);
    if (pit == positions_.end()) return 0;
    auto uit = pit->second.find(asset);
    if (uit == pit->second.end()) return 0;
    return (uint64_t)((__uint128_t)uit->second.supply_shares * mit->second.supply_index / WAD);
}
uint64_t LendingPool::BorrowBalance(const std::string& asset, const std::string& user) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto mit = markets_.find(asset);
    if (mit == markets_.end()) return 0;
    auto pit = positions_.find(user);
    if (pit == positions_.end()) return 0;
    auto uit = pit->second.find(asset);
    if (uit == pit->second.end()) return 0;
    return (uint64_t)((__uint128_t)uit->second.borrow_shares * mit->second.borrow_index / WAD);
}
uint32_t LendingPool::UtilizationBps(const std::string& asset) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = markets_.find(asset);
    if (it == markets_.end()) return 0;
    uint64_t avail = it->second.total_supply > it->second.reserves ? it->second.total_supply - it->second.reserves : 0;
    if (avail == 0) return 0;
    if (it->second.total_borrows >= avail) return BPS;
    return (uint32_t)(it->second.total_borrows * BPS / avail);
}
uint32_t LendingPool::BorrowRateBps(const std::string& asset) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = markets_.find(asset);
    if (it == markets_.end()) return 0;
    uint64_t avail = it->second.total_supply > it->second.reserves ? it->second.total_supply - it->second.reserves : 0;
    uint32_t U = 0;
    if (avail > 0) U = it->second.total_borrows >= avail ? BPS : (uint32_t)(it->second.total_borrows * BPS / avail);
    return ComputeBorrowRate(it->second, U);
}
uint32_t LendingPool::SupplyRateBps(const std::string& asset) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = markets_.find(asset);
    if (it == markets_.end()) return 0;
    uint64_t avail = it->second.total_supply > it->second.reserves ? it->second.total_supply - it->second.reserves : 0;
    uint32_t U = 0;
    if (avail > 0) U = it->second.total_borrows >= avail ? BPS : (uint32_t)(it->second.total_borrows * BPS / avail);
    uint32_t br = ComputeBorrowRate(it->second, U);
    uint64_t s = (uint64_t)br * U / BPS;
    s = s * (BPS - it->second.cfg.reserve_factor_bps) / BPS;
    return (uint32_t)s;
}
size_t LendingPool::MarketCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return markets_.size();
}

AccountInfo LendingPool::GetAccountInfo(const std::string& user,
                                        const std::map<std::string, uint64_t>& prices) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    AccountInfo info;
    auto pit = positions_.find(user);
    if (pit == positions_.end()) return info;

    for (auto& [asset, pos] : pit->second) {
        auto mit = markets_.find(asset);
        if (mit == markets_.end()) continue;
        auto pit2 = prices.find(asset);
        if (pit2 == prices.end()) continue;
        uint64_t price = pit2->second;

        uint64_t sup_bal = (uint64_t)((__uint128_t)pos.supply_shares * mit->second.supply_index / WAD);
        uint64_t bor_bal = (uint64_t)((__uint128_t)pos.borrow_shares * mit->second.borrow_index / WAD);

        __uint128_t cv = (__uint128_t)sup_bal * price;
        __uint128_t dv = (__uint128_t)bor_bal * price;
        __uint128_t cv_w = cv * mit->second.cfg.liquidation_threshold_bps / BPS;

        info.total_collateral_value += (uint64_t)cv_w;
        info.total_debt_value += (uint64_t)dv;
    }

    if (info.total_debt_value == 0) {
        info.health_factor_bps = UINT32_MAX;
        info.liquidatable = false;
    } else {
        __uint128_t hf = (__uint128_t)info.total_collateral_value * BPS / info.total_debt_value;
        info.health_factor_bps = hf > UINT32_MAX ? UINT32_MAX : (uint32_t)hf;
        info.liquidatable = info.health_factor_bps < BPS;
    }
    return info;
}

}