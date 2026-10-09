#include "test_framework.h"
#include "shivrai/gaming/assets.h"
using namespace shivrai::gaming;

TEST(mint_asset_basic) {
    GameAssetRegistry r;
    auto id = r.mint_asset("p1", "g1", "sword", "Excalibur", AssetRarity::LEGENDARY);
    ASSERT_FALSE(id.empty());
    ASSERT_EQ(r.get_asset(id)->owner, std::string("p1"));
    ASSERT_EQ(r.get_asset(id)->rarity, AssetRarity::LEGENDARY);
}

TEST(mint_asset_invalid) {
    GameAssetRegistry r;
    ASSERT_TRUE(r.mint_asset("", "g1", "s", "x", AssetRarity::COMMON).empty());
    ASSERT_TRUE(r.mint_asset("p", "", "s", "x", AssetRarity::COMMON).empty());
}

TEST(transfer_asset) {
    GameAssetRegistry r;
    auto id = r.mint_asset("p1", "g1", "sword", "x", AssetRarity::RARE);
    ASSERT_TRUE(r.transfer_asset(id, "p2"));
    ASSERT_EQ(r.get_asset(id)->owner, std::string("p2"));
}

TEST(lock_unlock) {
    GameAssetRegistry r;
    auto id = r.mint_asset("p1", "g1", "sword", "x", AssetRarity::RARE);
    ASSERT_TRUE(r.lock_asset(id));
    ASSERT_FALSE(r.transfer_asset(id, "p2"));
    ASSERT_TRUE(r.unlock_asset(id));
}

TEST(burn_asset) {
    GameAssetRegistry r;
    auto id = r.mint_asset("p1", "g1", "sword", "x", AssetRarity::COMMON);
    ASSERT_TRUE(r.burn_asset(id));
    ASSERT_EQ(r.get_asset(id)->status, AssetStatus::BURNED);
}

TEST(level_up) {
    GameAssetRegistry r;
    auto id = r.mint_asset("p1", "g1", "sword", "x", AssetRarity::COMMON);
    ASSERT_EQ(r.get_asset(id)->level, 1u);
    r.level_up(id, 2500);
    ASSERT_EQ(r.get_asset(id)->level, 3u);
}

TEST(record_reward) {
    GameAssetRegistry r;
    r.record_reward("p1", "g1", 100, "HON", "quest");
    r.record_reward("p1", "g1", 50, "HON", "pvp");
    ASSERT_EQ(r.rewards_by_player("p1").size(), 2u);
}

int main() {
    return shivrai::test::TestRunner::instance().run();
}
