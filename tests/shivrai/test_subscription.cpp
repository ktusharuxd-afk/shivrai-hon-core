#include "shivrai/subscription/subscription.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::subscription;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    Subscriptions s;
    CHECK(s.Count() == 0);
    CHECK(!s.Subscribe("", "netflix", "basic", 100, 30).has_value());
    CHECK(!s.Subscribe("alice", "", "basic", 100, 30).has_value());
    CHECK(!s.Subscribe("alice", "netflix", "", 100, 30).has_value());
    CHECK(!s.Subscribe("alice", "netflix", "basic", 0, 30).has_value());
    CHECK(!s.Subscribe("alice", "netflix", "basic", 100, 0).has_value());
    CHECK(!s.Subscribe("alice", "alice", "basic", 100, 30).has_value());
    auto s1 = s.Subscribe("alice", "netflix", "basic", 100, 30);
    CHECK(s1.has_value());
    CHECK(s.Count() == 1);
    auto sub = s.Get(*s1);
    CHECK(sub.has_value());
    CHECK(sub->subscriber == "alice");
    CHECK(sub->provider == "netflix");
    CHECK(sub->plan == "basic");
    CHECK(sub->amount == 100);
    CHECK(sub->interval_sec == 30);
    CHECK(sub->active);
    CHECK(sub->cycles_billed == 0);
    CHECK(sub->total_billed == 0);
    CHECK(s.SubsOf("alice").size() == 1);
    CHECK(s.SubsOf("bob").empty());
    auto paid = s.Charge(*s1);
    CHECK(paid == 100);
    CHECK(s.Get(*s1)->cycles_billed == 1);
    CHECK(s.Get(*s1)->total_billed == 100);
    CHECK(s.Charge(*s1) == 100);
    CHECK(s.Get(*s1)->total_billed == 200);
    CHECK(!s.Cancel(*s1, "bob"));
    CHECK(s.Pause(*s1, "alice"));
    CHECK(!s.Get(*s1)->active);
    CHECK(s.Charge(*s1) == 0);
    CHECK(!s.Pause(*s1, "alice"));
    CHECK(s.Resume(*s1, "alice"));
    CHECK(s.Get(*s1)->active);
    CHECK(!s.Resume(*s1, "alice"));
    CHECK(s.Cancel(*s1, "alice"));
    CHECK(!s.Get(*s1)->active);
    CHECK(!s.Cancel(*s1, "alice"));
    CHECK(!s.Get(99999).has_value());
    auto s2 = s.Subscribe("bob", "spotify", "premium", 200, 30);
    CHECK(s2.has_value());
    auto s3 = s.ChangePlan(*s2, "bob", "family", 300);
    CHECK(s3.has_value());
    CHECK(!s.Get(*s2)->active);
    CHECK(s.Get(*s3)->plan == "family");
    CHECK(s.Get(*s3)->amount == 300);
    CHECK(!s.ChangePlan(*s3, "alice", "x", 100).has_value());
    CHECK(!s.ChangePlan(*s3, "bob", "", 100).has_value());
    CHECK(!s.ChangePlan(*s3, "bob", "x", 0).has_value());
    CHECK(!s.ChangePlan(*s3, "bob", "family", 100).has_value());
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
