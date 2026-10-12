#include "shivrai/payment_stream/payment_stream.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::payment_stream;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    PaymentStream ps;
    CHECK(ps.Count() == 0);
    CHECK(!ps.Create("", "b", 100, 100).has_value());
    CHECK(!ps.Create("a", "b", 0, 100).has_value());
    CHECK(!ps.Create("a", "b", 100, 0).has_value());
    CHECK(!ps.Create("a", "a", 100, 100).has_value());
    auto s1 = ps.Create("alice", "bob", 1000, 100);
    CHECK(s1.has_value());
    CHECK(ps.Count() == 1);
    auto st = ps.Get(*s1);
    CHECK(st.has_value());
    CHECK(st->sender == "alice");
    CHECK(st->recipient == "bob");
    CHECK(st->total == 1000);
    CHECK(st->state == StreamState::Active);
    CHECK(ps.Claimable(*s1) == 0);
    CHECK(!ps.Withdraw(*s1).has_value());
    CHECK(!ps.Get(99999).has_value());
    CHECK(!ps.Pause(99999, "alice"));
    CHECK(!ps.Pause(*s1, "bob"));
    CHECK(ps.Pause(*s1, "alice"));
    CHECK(ps.Get(*s1)->state == StreamState::Paused);
    CHECK(!ps.Pause(*s1, "alice"));
    CHECK(ps.Resume(*s1, "alice"));
    CHECK(!ps.Resume(*s1, "alice"));
    CHECK(ps.Cancel(*s1, "alice"));
    CHECK(ps.Get(*s1)->state == StreamState::Cancelled);
    CHECK(!ps.Cancel(*s1, "alice"));
    CHECK(ps.Claimable(*s1) == 0);
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
