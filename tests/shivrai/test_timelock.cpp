#include "shivrai/timelock/timelock.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::timelock;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    Timelock tl; tl.SetDelaySec(0);
    CHECK(tl.DelaySec() == 0);
    CHECK(tl.Count() == 0);
    auto o = tl.Queue("alice", "setFee", "0x01");
    CHECK(o.has_value());
    CHECK(tl.Count() == 1);
    CHECK(tl.Get(*o)->proposer == "alice");
    CHECK(tl.Execute(*o));
    CHECK(tl.Get(*o)->state == OpState::Executed);
    CHECK(!tl.Execute(*o));
    CHECK(!tl.Queue("", "x", "y").has_value());
    CHECK(!tl.Get(99999).has_value());
    auto o2 = tl.Queue("bob", "upgrade", "0x02");
    CHECK(!tl.Cancel(*o2, "alice"));
    CHECK(tl.Cancel(*o2, "bob"));
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
