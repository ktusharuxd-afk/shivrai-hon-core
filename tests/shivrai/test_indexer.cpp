#include "shivrai/indexer/indexer.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::indexer;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    Indexer idx;
    CHECK(idx.Count() == 0);
    CHECK(idx.Index("", "k", "v") == 0);
    CHECK(idx.Index("cat", "", "v") == 0);
    auto a = idx.Index("tx", "sender", "alice");
    auto b = idx.Index("tx", "sender", "alice");
    auto c = idx.Index("tx", "sender", "bob");
    CHECK(a > 0 && b > 0 && c > 0);
    CHECK(idx.Count() == 3);
    auto la = idx.Lookup("tx", "sender");
    CHECK(la.size() == 3); // all share key
    auto lb = idx.Lookup("tx", "other");
    CHECK(lb.empty());
    auto ba = idx.ByCategory("tx");
    CHECK(ba.size() == 3);
    CHECK(idx.ByCategory("nope").empty());
    auto g = idx.Get(a);
    CHECK(g.has_value());
    CHECK(g->category == "tx");
    CHECK(g->key == "sender");
    CHECK(g->value == "alice");
    CHECK(!idx.Get(99999).has_value());
    auto ks = idx.KeysIn("tx");
    CHECK(ks.size() == 1);
    CHECK(ks[0] == "sender");
    CHECK(idx.Remove(a));
    CHECK(idx.Count() == 2);
    CHECK(!idx.Remove(a));
    CHECK(!idx.Remove(99999));
    auto la2 = idx.Lookup("tx", "sender");
    CHECK(la2.size() == 2);
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
