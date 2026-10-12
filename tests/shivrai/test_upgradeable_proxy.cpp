#include "shivrai/upgradeable_proxy/upgradeable_proxy.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::upgradeable_proxy;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    UpgradeableProxy p;
    CHECK(!p.IsInitialized());
    CHECK(p.CurrentHash().empty());
    CHECK(p.Admin().empty());
    CHECK(p.VersionCount() == 0);
    CHECK(!p.Initialize("", "0xhash1"));
    CHECK(!p.Initialize("alice", ""));
    CHECK(p.Initialize("alice", "0xhash1"));
    CHECK(p.IsInitialized());
    CHECK(p.Admin() == "alice");
    CHECK(p.CurrentHash() == "0xhash1");
    CHECK(p.VersionCount() == 1);
    CHECK(!p.Initialize("bob", "0xhash2")); // already initialized
    CHECK(!p.Upgrade("bob", "0xhash2", 1, 0, 0));
    CHECK(!p.Upgrade("alice", "", 1, 0, 0));
    CHECK(p.Upgrade("alice", "0xhash2", 1, 0, 0));
    CHECK(p.CurrentHash() == "0xhash2");
    CHECK(p.VersionCount() == 2);
    CHECK(p.Upgrade("alice", "0xhash3", 2, 0, 0));
    CHECK(p.VersionCount() == 3);
    CHECK(p.CurrentHash() == "0xhash3");
    auto v0 = p.VersionAt(0);
    CHECK(v0.has_value());
    CHECK(v0->impl_hash == "0xhash1");
    auto v2 = p.VersionAt(2);
    CHECK(v2.has_value());
    CHECK(v2->major == 2);
    CHECK(v2->impl_hash == "0xhash3");
    CHECK(!p.VersionAt(99).has_value());
    CHECK(p.Rollback("alice"));
    CHECK(p.CurrentHash() == "0xhash2");
    CHECK(p.VersionCount() == 2);
    CHECK(!p.Rollback("bob"));
    CHECK(!p.TransferAdmin("bob", "carol"));
    CHECK(!p.TransferAdmin("alice", ""));
    CHECK(!p.TransferAdmin("alice", "alice"));
    CHECK(p.TransferAdmin("alice", "carol"));
    CHECK(p.Admin() == "carol");
    CHECK(!p.Upgrade("alice", "0xhash4", 3, 0, 0));
    CHECK(p.Upgrade("carol", "0xhash4", 3, 0, 0));
    // rollback to only version
    UpgradeableProxy p2;
    p2.Initialize("x", "h1");
    CHECK(!p2.Rollback("x"));
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
