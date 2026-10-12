#include "shivrai/stablecoin/stablecoin.h"
#include <cassert>
#include <cstdint>
#include <iostream>

using namespace shivrai::stablecoin;

int main() {
    int passed = 0;
    auto CHECK = [&](bool c) { assert(c); ++passed; };

    Stablecoin sc;

    CHECK(sc.TotalSupply() == 0);
    CHECK(sc.VaultCount() == 0);
    CHECK(sc.PSMReserve() == 0);
    CHECK(sc.StabilityPoolBalance() == 0);
    CHECK(!sc.IsShutdown());
    CHECK(sc.MinCollateralBps() == 15000);
    CHECK(sc.LiquidationBps() == 13000);
    CHECK(sc.StabilityFeeBps() == 200);

    auto vid = sc.OpenVault("alice", "ETH", 10);
    CHECK(vid.has_value());
    CHECK(sc.VaultCount() == 1);
    auto v = sc.GetVault(*vid);
    CHECK(v.has_value());
    CHECK(v->owner == "alice");
    CHECK(v->collateral_amount == 10);
    CHECK(v->debt == 0);

    CHECK(sc.DepositCollateral(*vid, 5));
    CHECK(sc.GetVault(*vid)->collateral_amount == 15);
    CHECK(!sc.DepositCollateral(9999, 5));

    auto m = sc.MintStable(*vid, 900, 100);
    CHECK(m.has_value());
    CHECK(*m == 900);
    CHECK(sc.TotalSupply() == 900);
    CHECK(sc.GetVault(*vid)->debt == 900);

    CHECK(!sc.MintStable(*vid, 200, 100).has_value());
    auto vid2 = sc.OpenVault("bob", "ETH", 2);
    CHECK(vid2.has_value());
    CHECK(!sc.MintStable(*vid2, 50, 100).has_value());

    CHECK(!sc.WithdrawCollateral(*vid, 16, 100));

    CHECK(sc.RepayStable(*vid, 400, 100));
    CHECK(sc.GetVault(*vid)->debt == 500);
    CHECK(sc.TotalSupply() == 500);

    CHECK(sc.WithdrawCollateral(*vid, 10, 100));
    CHECK(sc.GetVault(*vid)->collateral_amount == 5);
    auto info = sc.GetVaultInfo(*vid, 100);
    CHECK(info.has_value());
    CHECK(info->collateral == 5);
    CHECK(info->debt == 500);
    CHECK(info->collateral_value == 500);
    CHECK(info->collateral_ratio_bps == 10000);
    CHECK(info->liquidatable);

    CHECK(sc.Liquidate(*vid, 100));
    CHECK(sc.VaultCount() == 1);
    CHECK(sc.TotalSupply() == 0);
    CHECK(sc.StabilityPoolDeposit("carol", 1000));
    CHECK(sc.StabilityPoolBalance() == 1000);
    CHECK(sc.StabilityPoolBalanceOf("carol") == 1000);
    CHECK(sc.StabilityPoolWithdraw("carol", 400));
    CHECK(sc.StabilityPoolBalance() == 600);

    sc.PSMDeposit(5000);
    CHECK(sc.PSMReserve() == 5000);
    auto s1 = sc.PSMSwapIn(1000);
    CHECK(s1.has_value() && *s1 == 1000);
    CHECK(sc.PSMReserve() == 4000);
    CHECK(sc.TotalSupply() == 1000);
    auto s2 = sc.PSMSwapOut(500);
    CHECK(s2.has_value() && *s2 == 500);
    CHECK(sc.PSMReserve() == 4500);
    CHECK(sc.TotalSupply() == 500);
    CHECK(!sc.PSMSwapOut(100000).has_value());

    sc.EmergencyShutdown();
    CHECK(sc.IsShutdown());
    CHECK(!sc.MintStable(*vid2, 100, 100).has_value());
    sc.Resume();
    CHECK(!sc.IsShutdown());
    CHECK(sc.TotalCollateral("ETH") == 2);

    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
