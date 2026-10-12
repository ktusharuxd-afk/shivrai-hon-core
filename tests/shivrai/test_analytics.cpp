#include "shivrai/analytics/analytics.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
using namespace shivrai::analytics;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    Analytics a;
    CHECK(a.SeriesCount() == 0);
    CHECK(a.Sum("x") == 0);
    CHECK(a.Count("x") == 0);
    CHECK(!a.Record("", 10));
    CHECK(a.Record("tx", 100));
    CHECK(a.Record("tx", 200));
    CHECK(a.Record("tx", 300));
    CHECK(a.SeriesCount() == 1);
    CHECK(a.Count("tx") == 3);
    CHECK(a.Sum("tx") == 600);
    CHECK(a.Max("tx") == 300);
    CHECK(a.Min("tx") == 100);
    CHECK(std::fabs(a.Avg("tx") - 200.0) < 1e-9);
    CHECK(a.Points("tx").size() == 3);
    a.Record("gas", 50);
    a.Record("gas", 70);
    CHECK(a.AllSeries().size() == 2);
    CHECK(a.Clear("gas"));
    CHECK(a.SeriesCount() == 1);
    CHECK(!a.Clear("gas"));
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
