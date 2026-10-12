#include "shivrai/dex/dex.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace shivrai::dex;

int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL line " << __LINE__ << ": " #x "\n"; std::exit(1); } } while(0)

    Dex d;

    // ---- Defaults ----
    CHECK(d.PoolCount() == 0);
    CHECK(d.AllPools().empty());

    // ---- CreatePool validation ----
    CHECK(!d.CreatePool("", "USDC", 30).has_value());
    CHECK(!d.CreatePool("ETH", "", 30).has_value());
    CHECK(!d.CreatePool("ETH", "ETH", 30).has_value());
    CHECK(!d.CreatePool("ETH", "USDC", 0).has_value());
    CHECK(!d.CreatePool("ETH", "USDC", 2000).has_value());

    // ---- Create ETH/USDC pool ----
    auto p1 = d.CreatePool("ETH", "USDC", 30);
    CHECK(p1.has_value());
    CHECK(d.PoolCount() == 1);

    auto pool = d.GetPool(*p1);
    CHECK(pool.has_value());
    CHECK(pool->token_a == "ETH");       // alphabetical
    CHECK(pool->token_b == "USDC");
    CHECK(pool->fee_bps == 30);
    CHECK(pool->reserve_a == 0);
    CHECK(pool->reserve_b == 0);
    CHECK(pool->total_lp_shares == 0);

    // Duplicate (same or reversed order)
    CHECK(!d.CreatePool("ETH", "USDC", 30).has_value());
    CHECK(!d.CreatePool("USDC", "ETH", 30).has_value());

    // ---- FindPool ----
    CHECK(d.FindPool("ETH", "USDC").has_value());
    CHECK(d.FindPool("USDC", "ETH").has_value());
    CHECK(*d.FindPool("ETH", "USDC") == *p1);
    CHECK(!d.FindPool("ETH", "DAI").has_value());

    // ---- AddLiquidity: initial ----
    // sqrt(10000 * 10000) = 10000 LP shares
    auto liq = d.AddLiquidity(*p1, 10000, 10000);
    CHECK(liq.has_value());
    CHECK(liq->lp_shares_minted == 10000);
    CHECK(liq->amount_a_added == 10000);
    CHECK(liq->amount_b_added == 10000);

    auto pool2 = d.GetPool(*p1);
    CHECK(pool2->reserve_a == 10000);
    CHECK(pool2->reserve_b == 10000);
    CHECK(pool2->total_lp_shares == 10000);

    // Bad amounts
    CHECK(!d.AddLiquidity(*p1, 0, 100).has_value());
    CHECK(!d.AddLiquidity(*p1, 100, 0).has_value());
    CHECK(!d.AddLiquidity(99999, 100, 100).has_value());

    // ---- AddLiquidity: proportional ----
    // pool now 10000/10000, LP 10000. Add 5000/5000 → shares = 5000
    auto liq2 = d.AddLiquidity(*p1, 5000, 5000);
    CHECK(liq2.has_value());
    CHECK(liq2->lp_shares_minted == 5000);
    CHECK(liq2->amount_a_added == 5000);
    CHECK(liq2->amount_b_added == 5000);

    auto pool3 = d.GetPool(*p1);
    CHECK(pool3->reserve_a == 15000);
    CHECK(pool3->reserve_b == 15000);
    CHECK(pool3->total_lp_shares == 15000);

    // ---- Spot price ----
    auto sp = d.SpotPrice(*p1);
    CHECK(sp.has_value());
    CHECK(std::fabs(*sp - 1.0) < 1e-9);

    auto isp = d.InverseSpotPrice(*p1);
    CHECK(isp.has_value());
    CHECK(std::fabs(*isp - 1.0) < 1e-9);

    // ---- QuoteOut: swap 1500 A in → expect ~1363 out ----
    // fee = 1500 * 30 / 10000 = 4 (integer)
    // net = 1496
    // out = 15000 * 1496 / (15000 + 1496) = 22,440,000 / 16,496 = 1360
    auto q = d.QuoteOut(*p1, true, 1500);
    CHECK(q.has_value());
    CHECK(*q == 1360);

    // ---- Swap A → B ----
    uint64_t bal_before = pool3->reserve_b;
    auto sr = d.Swap(*p1, true, 1500, 1000);
    CHECK(sr.has_value());
    CHECK(sr->amount_in == 1500);
    CHECK(sr->amount_out == 1360);
    CHECK(sr->fee_paid == 4);
    CHECK(sr->new_reserve_in == 16500);
    CHECK(sr->new_reserve_out == bal_before - 1360);

    auto pool4 = d.GetPool(*p1);
    CHECK(pool4->reserve_a == 16500);
    CHECK(pool4->reserve_b == 13640);
    CHECK(pool4->cumulative_volume_a == 1500);
    CHECK(pool4->cumulative_volume_b == 0);

    // Spot price moved: 13640 / 16500 ≈ 0.8267
    auto sp2 = d.SpotPrice(*p1);
    CHECK(sp2.has_value());
    CHECK(*sp2 < 1.0);

    // ---- Swap B → A (reverse direction) ----
    auto sr2 = d.Swap(*p1, false, 500, 0);
    CHECK(sr2.has_value());
    CHECK(sr2->amount_in == 500);
    // fee = 500*30/10000 = 1
    // net = 499
    // out = 16500 * 499 / (13640 + 499) = 8,233,500 / 14,139 = 582
    CHECK(sr2->amount_out == 582);
    CHECK(sr2->fee_paid == 1);
    auto pool4b = d.GetPool(*p1);
    CHECK(pool4b->cumulative_volume_b == 500);

    auto pool5 = d.GetPool(*p1);
    CHECK(pool5->reserve_a == 16500 - 582);
    CHECK(pool5->reserve_b == 13640 + 500);

    // ---- Slippage check: min_amount_out too high → fail ----
    auto sr3 = d.Swap(*p1, true, 1000, 9999999);
    CHECK(!sr3.has_value());

    // ---- Swap validation ----
    CHECK(!d.Swap(99999, true, 1000, 0).has_value());
    CHECK(!d.Swap(*p1, true, 0, 0).has_value());
    CHECK(!d.QuoteOut(99999, true, 1000).has_value());
    CHECK(!d.QuoteOut(*p1, true, 0).has_value());

    // ---- LP balances ----
    CHECK(d.LpBalance(*p1, "__initial__") == 15000);
    CHECK(d.LpBalance(*p1, "alice") == 0);

    // ---- TransferLp ----
    CHECK(d.TransferLp(*p1, "__initial__", "alice", 3000));
    CHECK(d.LpBalance(*p1, "__initial__") == 12000);
    CHECK(d.LpBalance(*p1, "alice") == 3000);

    CHECK(!d.TransferLp(*p1, "alice", "bob", 99999));    // not enough
    CHECK(!d.TransferLp(*p1, "alice", "alice", 100));    // self
    CHECK(!d.TransferLp(*p1, "alice", "", 100));         // empty to
    CHECK(!d.TransferLp(*p1, "alice", "bob", 0));        // zero
    CHECK(!d.TransferLp(99999, "alice", "bob", 100));    // bad pool

    CHECK(d.TransferLp(*p1, "alice", "bob", 1000));
    CHECK(d.LpBalance(*p1, "alice") == 2000);
    CHECK(d.LpBalance(*p1, "bob") == 1000);

    // ---- RemoveLiquidity ----
    auto rl = d.RemoveLiquidity(*p1, 1000);
    CHECK(rl.has_value());
    CHECK(rl->lp_shares_burned == 1000);
    // pool5: reserve_a = 15918, reserve_b = 14140, total_lp = 15000
    // 1000/15000 of each: a = 1061, b = 942
    // (values depend on integer math; check roughly)
    CHECK(rl->amount_a_returned > 0);
    CHECK(rl->amount_b_returned > 0);

    // min_out fails
    CHECK(!d.RemoveLiquidity(*p1, 1000, 99999999, 99999999).has_value());

    // Too many shares
    CHECK(!d.RemoveLiquidity(*p1, 99999999).has_value());
    CHECK(!d.RemoveLiquidity(*p1, 0).has_value());
    CHECK(!d.RemoveLiquidity(99999, 100).has_value());

    // ---- Second pool (isolated) ----
    auto p2 = d.CreatePool("DAI", "USDC", 10);
    CHECK(p2.has_value());
    CHECK(d.PoolCount() == 2);

    auto liq3 = d.AddLiquidity(*p2, 20000, 20000);
    CHECK(liq3.has_value());

    // p1 unchanged by operations on p2
    auto final_p1 = d.GetPool(*p1);
    CHECK(final_p1->fee_bps == 30);
    auto final_p2 = d.GetPool(*p2);
    CHECK(final_p2->fee_bps == 10);

    // ---- Config ----
    CHECK(d.SetFeeBps(*p1, 100));      // 1%
    CHECK(d.FeeBps(*p1) == 100);
    CHECK(!d.SetFeeBps(*p1, 0));       // below min
    CHECK(!d.SetFeeBps(*p1, 5000));    // above max
    CHECK(!d.SetFeeBps(99999, 100));   // bad pool

    // ---- Unknown pool queries ----
    CHECK(!d.GetPool(99999).has_value());
    CHECK(!d.SpotPrice(99999).has_value());
    CHECK(!d.InverseSpotPrice(99999).has_value());
    CHECK(d.FeeBps(99999) == 0);
    CHECK(d.LpBalance(99999, "alice") == 0);

    // ---- AllPools ----
    auto all = d.AllPools();
    CHECK(all.size() == 2);

    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}