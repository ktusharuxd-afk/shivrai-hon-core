#include "shivrai/oracle/oracle.h"

#include <cassert>
#include <ctime>
#include <iostream>

using namespace shivrai::oracle;

int main() {
    OracleRegistry reg;
    reg.SetMinSources(2);
    reg.SetHeartbeatSeconds(300);
    reg.SetMaxDeviationBps(2000);  // 20%

    // ---- Register oracles ----
    assert(reg.RegisterOracle("oracle-a"));
    assert(reg.RegisterOracle("oracle-b"));
    assert(reg.RegisterOracle("oracle-c"));
    assert(!reg.RegisterOracle("oracle-a"));  // duplicate
    assert(reg.RegisterOracle("oracle-a") == false);
    assert(reg.IsAuthorized("oracle-a"));
    assert(!reg.IsAuthorized("nobody"));
    assert(reg.ListOracles().size() == 3);

    // ---- Unregister ----
    assert(reg.RegisterOracle("temp"));
    assert(reg.UnregisterOracle("temp"));
    assert(!reg.UnregisterOracle("temp"));

    // ---- Submit prices ----
    int64_t now = (int64_t)std::time(nullptr);
    assert(reg.SubmitPrice("HON/USD", 100000, now, "oracle-a"));
    assert(reg.SubmitPrice("HON/USD", 101000, now, "oracle-b"));

    auto price = reg.GetPrice("HON/USD");
    assert(price.has_value());
    assert(*price == (100000 + 101000) / 2);  // median of 2 = avg

    // ---- Unauthorized ----
    assert(!reg.SubmitPrice("HON/USD", 999, now, "hacker"));
    assert(!reg.SubmitPrice("HON/USD", 0, now, "oracle-a"));  // zero price

    // ---- Deviation check ----
    // Current median ~100500. 20% deviation = ~20100. Try 200000 — too far.
    assert(!reg.SubmitPrice("HON/USD", 200000, now, "oracle-c"));

    // Slight move OK: 110000 (within 20%)
    assert(reg.SubmitPrice("HON/USD", 110000, now, "oracle-c"));

    // ---- Third oracle: median recalculated ----
    auto price2 = reg.GetPrice("HON/USD");
    assert(price2.has_value());

    // ---- Insufficient sources ----
    OracleRegistry reg2;
    reg2.SetMinSources(3);
    reg2.RegisterOracle("o1");
    reg2.SubmitPrice("BTC/USD", 5000000, (int64_t)std::time(nullptr), "o1");
    // Only 1 source, need 3 → no price yet
    assert(!reg2.GetPrice("BTC/USD").has_value());

    // ---- Feed info ----
    auto feed = reg.GetFeed("HON/USD");
    assert(feed.has_value());
    assert(feed->source_count == 3);
    assert(!feed->is_stale);

    // ---- Staleness ----
    assert(reg.IsStale("UNKNOWN/PAIR", now + 100));  // no such feed
    assert(!reg.IsStale("HON/USD", now + 10));

    // ---- History ----
    auto hist = reg.GetHistory("HON/USD");
    assert(hist.size() == 3);

    auto hist_limited = reg.GetHistory("HON/USD", 2);
    assert(hist_limited.size() == 2);

    // ---- Emergency pause ----
    reg.Pause();
    assert(reg.IsPaused());
    assert(!reg.SubmitPrice("HON/USD", 105000, now, "oracle-a"));

    reg.Resume();
    assert(!reg.IsPaused());
    assert(reg.SubmitPrice("HON/USD", 105000, now + 10, "oracle-a"));

    // ---- Prune ----
    reg.PruneStale(now + 10000);  // 10000s later -> all pruned
    auto hist_after = reg.GetHistory("HON/USD");
    (void)hist_after;

    // ---- Multiple assets ----
    reg.SubmitPrice("BTC/USD", 50000000, now, "oracle-a");
    reg.SubmitPrice("BTC/USD", 51000000, now, "oracle-b");
    auto btc = reg.GetPrice("BTC/USD");
    assert(btc.has_value());

    assert(reg.FeedCount() == 2);
    assert(reg.TotalUpdates() == 6);

    // ---- Future timestamp rejected ----
    OracleRegistry reg3;
    reg3.SetMinSources(1);
    reg3.RegisterOracle("x");
    // Very far future
    int64_t far_future = now + 10000;
    assert(!reg3.SubmitPrice("A/B", 100, far_future, "x"));

    // ---- Same reporter updates ----
    OracleRegistry reg4;
    reg4.SetMinSources(1);
    reg4.RegisterOracle("p");
    assert(reg4.SubmitPrice("Z/Z", 1000, now - 100, "p"));
    assert(reg4.SubmitPrice("Z/Z", 1100, now - 50, "p"));   // newer, OK
    assert(!reg4.SubmitPrice("Z/Z", 900, now - 200, "p"));  // older, reject

    std::cout << "========================================\n";
    std::cout << "  ALL ORACLE TESTS PASSED\n";
    std::cout << "========================================\n";
    return 0;
}
