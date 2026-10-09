#include "test_framework.h"
#include "shivrai/assets/rwa.h"
using namespace shivrai::assets;

TEST(register_asset_basic) {
    RWARegistry r;
    auto id = r.register_asset("alice", RWAType::REAL_ESTATE, "Apt", 5000, "USD", "i", "IN");
    ASSERT_FALSE(id.empty());
    ASSERT_EQ(r.count(), 1u);
    ASSERT_EQ(r.get(id)->owner, std::string("alice"));
}

TEST(register_asset_invalid) {
    RWARegistry r;
    ASSERT_TRUE(r.register_asset("", RWAType::REAL_ESTATE, "x", 1, "U", "i", "IN").empty());
    ASSERT_TRUE(r.register_asset("a", RWAType::REAL_ESTATE, "x", 0, "U", "i", "IN").empty());
}

TEST(activate_asset) {
    RWARegistry r;
    auto id = r.register_asset("a", RWAType::REAL_ESTATE, "x", 100, "USD", "i", "IN");
    ASSERT_TRUE(r.activate(id));
    ASSERT_EQ(r.get(id)->status, RWAStatus::ACTIVE);
    ASSERT_FALSE(r.activate(id));
}

TEST(transfer_asset) {
    RWARegistry r;
    auto id = r.register_asset("a", RWAType::REAL_ESTATE, "x", 100, "USD", "i", "IN");
    ASSERT_FALSE(r.transfer(id, "b", "tx1"));
    r.activate(id);
    ASSERT_TRUE(r.transfer(id, "b", "tx1"));
    ASSERT_EQ(r.transfer_history(id).size(), 1u);
}

TEST(freeze_and_redeem) {
    RWARegistry r;
    auto id = r.register_asset("a", RWAType::REAL_ESTATE, "x", 100, "USD", "i", "IN");
    ASSERT_TRUE(r.freeze(id));
    ASSERT_EQ(r.get(id)->status, RWAStatus::FROZEN);
    auto id2 = r.register_asset("a", RWAType::INVOICE, "y", 200, "USD", "i", "IN");
    r.activate(id2);
    ASSERT_TRUE(r.redeem(id2));
    ASSERT_EQ(r.get(id2)->status, RWAStatus::REDEEMED);
}

TEST(assets_by_owner) {
    RWARegistry r;
    r.register_asset("a", RWAType::REAL_ESTATE, "x", 1, "USD", "i", "IN");
    r.register_asset("a", RWAType::INVOICE, "y", 2, "USD", "i", "IN");
    r.register_asset("b", RWAType::COMMODITY, "z", 3, "USD", "i", "IN");
    ASSERT_EQ(r.by_owner("a").size(), 2u);
}

int main() {
    return shivrai::test::TestRunner::instance().run();
}
