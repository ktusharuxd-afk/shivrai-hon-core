#include "shivrai/airdrop/airdrop.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::airdrop;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    Airdrop a;
    CHECK(a.RoundCount() == 0);
    CHECK(!a.CreateRound("", 0, 100).has_value());
    CHECK(!a.CreateRound("Drop1", 100, 50).has_value());
    // Round active now → use past/future large range
    auto r1 = a.CreateRound("Drop1", -100000, 9999999999LL);
    CHECK(r1.has_value());
    CHECK(a.RoundCount() == 1);
    auto rd = a.GetRound(*r1);
    CHECK(rd.has_value());
    CHECK(rd->name == "Drop1");
    CHECK(rd->total_allocated == 0);
    CHECK(rd->total_claimed == 0);
    CHECK(rd->active);
    CHECK(!a.SetEligibility(*r1, "", 100));
    CHECK(!a.SetEligibility(*r1, "alice", 0));
    CHECK(a.SetEligibility(*r1, "alice", 1000));
    CHECK(a.SetEligibility(*r1, "bob", 500));
    CHECK(!a.SetEligibility(*r1, "alice", 200)); // dup
    CHECK(a.GetRound(*r1)->total_allocated == 1500);
    auto amt = a.AmountFor(*r1, "alice");
    CHECK(amt.has_value() && *amt == 1000);
    CHECK(!a.AmountFor(*r1, "carol").has_value());
    CHECK(!a.AmountFor(99999, "alice").has_value());
    CHECK(!a.HasClaimed(*r1, "alice"));
    auto c = a.Claim(*r1, "alice");
    CHECK(c.has_value() && *c == 1000);
    CHECK(a.HasClaimed(*r1, "alice"));
    CHECK(!a.Claim(*r1, "alice"));    // double claim
    CHECK(a.GetRound(*r1)->total_claimed == 1000);
    CHECK(!a.Claim(*r1, "carol"));    // not eligible
    CHECK(!a.Claim(99999, "alice"));  // bad round
    // Round with future start → cannot claim
    auto r3 = a.CreateRound("Future", 9999999999LL, 9999999999LL + 100000);
    CHECK(r3.has_value());
    CHECK(a.SetEligibility(*r3, "alice", 100));
    CHECK(!a.Claim(*r3, "alice")); // not started
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
