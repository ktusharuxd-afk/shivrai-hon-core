#include "shivrai/oracle_aggregator/oracle_aggregator.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::oracle_aggregator;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    OracleAggregator oa;
    CHECK(oa.SourceCount() == 0);
    CHECK(oa.MaxAgeSec() == 300);
    CHECK(!oa.RegisterSource(""));
    CHECK(oa.RegisterSource("binance"));
    CHECK(oa.RegisterSource("kraken"));
    CHECK(oa.RegisterSource("coinbase"));
    CHECK(!oa.RegisterSource("binance"));
    CHECK(oa.SourceCount() == 3);
    CHECK(!oa.SubmitPrice("", "BTC", 100));
    CHECK(!oa.SubmitPrice("binance", "", 100));
    CHECK(!oa.SubmitPrice("binance", "BTC", 0));
    CHECK(!oa.SubmitPrice("unknown", "BTC", 100));
    CHECK(oa.SubmitPrice("binance", "BTC", 100));
    CHECK(oa.SubmitPrice("kraken", "BTC", 102));
    CHECK(oa.SubmitPrice("coinbase", "BTC", 98));
    CHECK(oa.PendingCount("BTC") == 3);
    CHECK(oa.PendingCount("ETH") == 0);
    auto agg = oa.Aggregate("BTC");
    CHECK(agg.has_value());
    CHECK(agg->asset == "BTC");
    CHECK(agg->sources == 3);
    CHECK(agg->median == 100);
    CHECK(agg->min_price == 98);
    CHECK(agg->max_price == 102);
    CHECK(agg->valid);
    CHECK(!oa.Aggregate("NOPE").has_value());
    oa.SubmitPrice("binance", "BTC", 200);
    auto agg2 = oa.Aggregate("BTC");
    CHECK(agg2->max_price == 200);
    CHECK(oa.RemoveSource("coinbase"));
    CHECK(oa.SourceCount() == 2);
    auto agg3 = oa.Aggregate("BTC");
    CHECK(agg3->sources == 2);
    CHECK(!oa.RemoveSource("coinbase"));
    oa.SetMaxAgeSec(10);
    CHECK(oa.MaxAgeSec() == 10);
    oa.SetMaxAgeSec(0);
    CHECK(oa.MaxAgeSec() == 10);
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
