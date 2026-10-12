#include "shivrai/randomness_beacon/randomness_beacon.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::randomness_beacon;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    Beacon b;
    CHECK(b.RoundCount() == 0);
    CHECK(!b.OpenRound("").has_value());
    auto r1 = b.OpenRound("seed1");
    CHECK(r1.has_value());
    CHECK(b.RoundCount() == 1);
    CHECK(b.GetRound(*r1)->seed == "seed1");
    CHECK(b.GetRound(*r1)->finalized == false);
    CHECK(!b.Commit(*r1, "", "c"));
    CHECK(!b.Commit(*r1, "alice", ""));
    CHECK(b.Commit(*r1, "alice", "H1"));
    CHECK(!b.Commit(*r1, "alice", "H2"));
    CHECK(b.Commit(*r1, "bob", "H2"));
    CHECK(!b.Reveal(*r1, "carol", "v"));  // no commitment
    CHECK(b.Reveal(*r1, "alice", "s1"));
    CHECK(!b.Reveal(*r1, "alice", "s1b")); // dup
    CHECK(b.Reveal(*r1, "bob", "s2"));
    CHECK(b.Finalize(*r1));
    CHECK(b.GetRound(*r1)->finalized);
    CHECK(!b.Commit(*r1, "carol", "H3"));
    CHECK(!b.Reveal(*r1, "carol", "s3"));
    CHECK(!b.Finalize(*r1));
    auto result = b.GetRound(*r1)->result;
    CHECK(!result.empty());
    CHECK(b.Combine(*r1) == result);
    auto r2 = b.OpenRound("seed2");
    CHECK(!b.Finalize(*r2)); // no reveals
    CHECK(!b.GetRound(99999).has_value());
    CHECK(b.Combine(99999) == "");
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
