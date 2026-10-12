#include "shivrai/lending/lending.h"
#include <cassert>
#include <iostream>

using namespace shivrai::lending;

int main() {
    int passed = 0;
    auto CHECK = [&](bool c) { assert(c); ++passed; };

    LendingPool pool;

    CHECK(pool.MarketCount() == 0);
    CHECK(!pool.MarketExists("USDC"));
    CHECK(pool.CreateMarket("USDC"));
    CHECK(pool.MarketExists("USDC"));
    CHECK(pool.MarketCount() == 1);
    CHECK(!pool.CreateMarket("USDC"));

    auto cfg = pool.Config("USDC");
    CHECK(cfg.has_value());
    CHECK(cfg->collateral_factor_bps == 7500);
    CHECK(cfg->liquidation_threshold_bps == 8000);
    CHECK(cfg->reserve_factor_bps == 1000);
    CHECK(cfg->optimal_utilization_bps == 8000);

    auto s1 = pool.Supply("USDC", "alice", 10000);
    CHECK(s1.has_value());
    CHECK(pool.TotalSupply("USDC") == 10000);
    CHECK(pool.SupplyBalance("USDC", "alice") == 10000);
    CHECK(pool.UtilizationBps("USDC") == 0);

    CHECK(pool.Supply("USDC", "bob", 5000).has_value());
    CHECK(pool.TotalSupply("USDC") == 15000);

    auto b1 = pool.Borrow("USDC", "carol", 6000);
    CHECK(b1.has_value());
    CHECK(pool.TotalBorrows("USDC") == 6000);
    CHECK(pool.BorrowBalance("USDC", "carol") == 6000);
    CHECK(pool.UtilizationBps("USDC") == 4000);
    CHECK(pool.BorrowRateBps("USDC") == 200);

    CHECK(!pool.Borrow("USDC", "dave", 10000).has_value());

    auto r1 = pool.Repay("USDC", "carol", 2000);
    CHECK(r1.has_value());
    CHECK(*r1 == 2000);
    CHECK(pool.TotalBorrows("USDC") == 4000);
    CHECK(pool.BorrowBalance("USDC", "carol") == 4000);

    auto w1 = pool.Withdraw("USDC", "alice", 5000);
    CHECK(w1.has_value());
    CHECK(*w1 == 5000);
    CHECK(pool.TotalSupply("USDC") == 10000);
    CHECK(pool.SupplyBalance("USDC", "alice") == 5000);

    CHECK(pool.SetCollateralFactor("USDC", 6000));
    CHECK(pool.Config("USDC")->collateral_factor_bps == 6000);
    CHECK(!pool.SetCollateralFactor("USDC", 20000));
    CHECK(!pool.SetCollateralFactor("NOPE", 5000));

    CHECK(pool.SetReserveFactor("USDC", 2000));
    CHECK(pool.Config("USDC")->reserve_factor_bps == 2000);

    CHECK(pool.SetInterestModel("USDC", 100, 500, 5000, 7000));
    CHECK(pool.Config("USDC")->optimal_utilization_bps == 7000);

    CHECK(!pool.SetInterestModel("USDC", 0, 0, 0, 0));

    std::map<std::string, uint64_t> prices;
    prices["USDC"] = 1;
    auto ai = pool.GetAccountInfo("alice", prices);
    CHECK(ai.total_debt_value == 0);
    CHECK(ai.health_factor_bps == UINT32_MAX);
    CHECK(!ai.liquidatable);

    auto ci = pool.GetAccountInfo("carol", prices);
    CHECK(ci.total_debt_value == 4000);
    CHECK(ci.liquidatable);

    pool.AccrueInterest("USDC");
    CHECK(pool.TotalSupply("USDC") == 10000);
    CHECK(pool.TotalBorrows("USDC") == 4000);

    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}