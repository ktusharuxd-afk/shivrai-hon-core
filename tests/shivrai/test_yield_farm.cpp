#include "shivrai/yield_farm/yield_farm.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::yield_farm;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    YieldFarm f;
    CHECK(f.PoolCount() == 0);
    CHECK(f.RewardRate() == 0);
    CHECK(!f.AddPool("", 100).has_value());
    CHECK(!f.AddPool("LP1", 0).has_value());
    auto p1 = f.AddPool("LP1", 100);
    auto p2 = f.AddPool("LP2", 200);
    CHECK(p1.has_value() && p2.has_value());
    CHECK(f.PoolCount() == 2);
    CHECK(!f.AddPool("LP1", 100).has_value());
    CHECK(f.GetPool(*p1)->lp_token == "LP1");
    CHECK(f.GetPool(*p1)->alloc_point == 100);
    CHECK(f.SetRewardRate(1000));
    CHECK(f.RewardRate() == 1000);
    CHECK(f.Stake(*p1, "alice", 5000));
    CHECK(f.StakedOf(*p1, "alice") == 5000);
    CHECK(f.GetPool(*p1)->total_staked == 5000);
    CHECK(!f.Stake(*p1, "", 100));
    CHECK(!f.Stake(*p1, "bob", 0));
    CHECK(!f.Stake(99999, "alice", 100));
    CHECK(f.Stake(*p1, "bob", 3000));
    CHECK(f.GetPool(*p1)->total_staked == 8000);
    CHECK(f.Pending(*p1, "alice") == 0);
    auto w = f.Withdraw(*p1, "alice", 2000);
    CHECK(w.has_value() && *w == 2000);
    CHECK(f.StakedOf(*p1, "alice") == 3000);
    CHECK(!f.Withdraw(*p1, "alice", 99999).has_value());
    CHECK(!f.Withdraw(*p1, "nope", 100).has_value());
    CHECK(f.Harvest(*p1, "alice").has_value());
    CHECK(!f.GetPool(99999).has_value());
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
