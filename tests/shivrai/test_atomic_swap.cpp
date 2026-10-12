#include "shivrai/atomic_swap/atomic_swap.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::atomic_swap;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    AtomicSwap as;
    CHECK(as.Count() == 0);
    CHECK(!as.Initiate("", "b", "eth", "shv", "USDC", 100, "0xhash", 100).has_value());
    CHECK(!as.Initiate("a", "b", "eth", "eth", "USDC", 100, "0xhash", 100).has_value());
    CHECK(!as.Initiate("a", "b", "eth", "shv", "", 100, "0xhash", 100).has_value());
    CHECK(!as.Initiate("a", "b", "eth", "shv", "USDC", 0, "0xhash", 100).has_value());
    CHECK(!as.Initiate("a", "b", "eth", "shv", "USDC", 100, "", 100).has_value());
    CHECK(!as.Initiate("a", "b", "eth", "shv", "USDC", 100, "0xhash", 0).has_value());
    auto s = as.Initiate("alice", "bob", "eth", "shv", "USDC", 1000, "0xabc", 3600);
    CHECK(s.has_value());
    CHECK(as.Count() == 1);
    auto sw = as.Get(*s);
    CHECK(sw.has_value());
    CHECK(sw->initiator == "alice");
    CHECK(sw->participant == "bob");
    CHECK(sw->amount == 1000);
    CHECK(sw->hash_lock == "0xabc");
    CHECK(sw->state == SwapState::Open);
    CHECK(!as.Claim(*s, "alice", "secret"));
    CHECK(!as.Claim(*s, "bob", ""));
    CHECK(as.Claim(*s, "bob", "secret"));
    CHECK(as.Get(*s)->state == SwapState::Claimed);
    CHECK(!as.Claim(*s, "bob", "again"));
    CHECK(!as.Refund(*s, "alice"));
    CHECK(!as.Get(99999).has_value());
    auto s2 = as.Initiate("c", "d", "eth", "shv", "USDC", 500, "0xdef", 1);
    CHECK(s2.has_value());
    CHECK(!as.Refund(*s2, "x"));
    CHECK(!as.Refund(*s2, "c"));
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
