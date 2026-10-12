#include "shivrai/event_log/event_log.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::event_log;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    EventLog l;
    CHECK(l.Count() == 0);
    CHECK(l.LastId() == 0);
    CHECK(l.Emit("", "x") == 0);
    auto a = l.Emit("transfer", "alice->bob 100");
    auto b = l.Emit("mint", "carol +50");
    auto c = l.Emit("transfer", "dave->eve 20");
    CHECK(a == 1 && b == 2 && c == 3);
    CHECK(l.Count() == 3);
    CHECK(l.TopicCount() == 2);
    CHECK(l.LastId() == 3);
    CHECK(l.Get(a).has_value());
    CHECK(l.Get(a)->topic == "transfer");
    CHECK(!l.Get(99999).has_value());
    CHECK(l.ByTopic("transfer").size() == 2);
    CHECK(l.ByTopic("nope").empty());
    CHECK(l.Range(1, 2).size() == 2);
    CHECK(l.Range(2, 3).size() == 2);
    CHECK(l.Range(99, 99).empty());
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
