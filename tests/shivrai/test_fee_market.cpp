#include "shivrai/fee_market/fee_market.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::fee_market;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    FeeMarket fm;
    CHECK(fm.CurrentBaseFee() == GWEI);
    CHECK(fm.BaseFeeFor(15'000'000, 1000) == 1000); // target
    CHECK(fm.BaseFeeFor(30'000'000, 1000) == 1125); // double → +12.5%
    CHECK(fm.BaseFeeFor(0, 1000) == 875);           // empty → -12.5%
    CHECK(fm.PendingCount() == 0);
    CHECK(fm.BlocksMined() == 0);
    CHECK(!fm.SubmitTx("", 100, 1, 21000));
    CHECK(!fm.SubmitTx("0x1", 100, 1, 0));
    CHECK(fm.SubmitTx("0x1", 200 * GWEI, 2 * GWEI, 21000));
    CHECK(fm.SubmitTx("0x2", 200 * GWEI, 3 * GWEI, 21000));
    CHECK(!fm.SubmitTx("0x1", 200 * GWEI, 2 * GWEI, 21000)); // dup
    CHECK(fm.PendingCount() == 2);
    CHECK(fm.GetTx("0x1").has_value());
    CHECK(fm.GetTx("0x1")->max_fee == 200 * GWEI);
    CHECK(!fm.GetTx("nope").has_value());
    CHECK(fm.EffectiveTip(200 * GWEI, 2 * GWEI, GWEI) == 2 * GWEI);
    CHECK(fm.EffectiveTip(GWEI, 2 * GWEI, 200 * GWEI) == 0);  // max_fee < base
    CHECK(fm.EffectiveTip(2 * GWEI, 5 * GWEI, GWEI) == GWEI); // capped by available
    CHECK(fm.MineBlock());
    CHECK(fm.BlocksMined() == 1);
    CHECK(fm.PendingCount() == 0);
    CHECK(fm.LastBlockGasUsed() == 42000);
    CHECK(fm.CurrentBaseFee() < GWEI);  // < target → dropped
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
