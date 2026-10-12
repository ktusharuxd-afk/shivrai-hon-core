#include "shivrai/gaming/gaming.h"
#include <cassert>
#include <cstdlib>
#include <iostream>

using namespace shivrai::gaming;

int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL line " << __LINE__ << ": " #x "\n"; std::exit(1); } } while(0)

    GameEngine g;

    CHECK(g.PlayerCount() == 0);
    CHECK(g.QuestCount() == 0);
    CHECK(g.ItemCount() == 0);

    // ---- Player registration ----
    CHECK(g.RegisterPlayer("alice"));
    CHECK(g.RegisterPlayer("bob"));
    CHECK(g.RegisterPlayer("carol"));
    CHECK(g.PlayerCount() == 3);
    CHECK(!g.RegisterPlayer("alice"));   // dup
    CHECK(!g.RegisterPlayer(""));        // empty

    auto ap = g.GetPlayer("alice");
    CHECK(ap.has_value());
    CHECK(ap->username == "alice");
    CHECK(ap->level == 1);
    CHECK(ap->xp == 0);
    CHECK(ap->gold == 0);
    CHECK(ap->pvp_wins == 0);
    CHECK(ap->pvp_losses == 0);

    CHECK(!g.GetPlayer("nope").has_value());

    // ---- XP curve ----
    // XpForLevel(1) = 0, (2) = 100, (3) = 100+120 = 220, (4) = 220+144 = 364
    CHECK(g.XpForLevel(1) == 0);
    CHECK(g.XpForLevel(2) == 100);
    CHECK(g.XpForLevel(3) == 220);
    CHECK(g.XpForLevel(4) == 364);

    CHECK(g.XpToNextLevel("alice") == 100);
    CHECK(g.XpToNextLevel("nope") == 0);

    // ---- Grant XP + level up ----
    CHECK(g.GrantXp("alice", 100));
    auto ap2 = g.GetPlayer("alice");
    CHECK(ap2->level == 2);
    CHECK(ap2->xp == 100);
    CHECK(g.XpToNextLevel("alice") == 120);   // need 220 total, have 100 → 120

    CHECK(g.GrantXp("alice", 200));
    auto ap3 = g.GetPlayer("alice");
    CHECK(ap3->level == 3);
    CHECK(ap3->xp == 300);

    CHECK(!g.GrantXp("nope", 100));
    CHECK(!g.GrantXp("bob", 0));

    // ---- Gold ----
    CHECK(g.GrantGold("bob", 500));
    CHECK(g.GetPlayer("bob")->gold == 500);
    CHECK(!g.GrantGold("nope", 100));
    CHECK(!g.GrantGold("bob", 0));

    // ---- Items ----
    ItemId sword = g.MintItem("alice", "Excalibur", ItemRarity::Legendary, 100, 50, false);
    ItemId shield = g.MintItem("alice", "Aegis",     ItemRarity::Epic,       10, 200, false);
    ItemId bound = g.MintItem("bob",   "Soul Ring",  ItemRarity::Rare,        5,   5, true);
    CHECK(sword != 0 && shield != 0 && bound != 0);
    CHECK(g.ItemCount() == 3);

    auto si = g.GetItem(sword);
    CHECK(si.has_value());
    CHECK(si->name == "Excalibur");
    CHECK(si->rarity == ItemRarity::Legendary);
    CHECK(si->attack == 100);
    CHECK(si->defense == 50);
    CHECK(si->owner == "alice");
    CHECK(!si->bound);

    CHECK(g.ItemsOf("alice").size() == 2);
    CHECK(g.ItemsOf("bob").size() == 1);
    CHECK(g.ItemsOf("carol").empty());

    // Transfer item
    CHECK(g.TransferItem("alice", "carol", sword));
    CHECK(g.GetItem(sword)->owner == "carol");
    CHECK(g.ItemsOf("alice").size() == 1);
    CHECK(g.ItemsOf("carol").size() == 1);

    // Cannot transfer bound
    CHECK(!g.TransferItem("bob", "carol", bound));

    // Cannot transfer to self
    CHECK(!g.TransferItem("carol", "carol", sword));

    // Wrong owner
    CHECK(!g.TransferItem("alice", "bob", sword));

    // Unknown item
    CHECK(!g.TransferItem("alice", "bob", 99999));

    // Zero return on bad input
    CHECK(g.MintItem("", "X", ItemRarity::Common, 0, 0, false) == 0);
    CHECK(g.MintItem("alice", "", ItemRarity::Common, 0, 0, false) == 0);

    // ---- Quests ----
    auto q1 = g.CreateQuest("Goblin Hunt", 1, 50, 100,
                            ItemRarity::Rare, 5000, 0);
    CHECK(q1.has_value());
    CHECK(g.QuestCount() == 1);

    auto qq = g.GetQuest(*q1);
    CHECK(qq.has_value());
    CHECK(qq->name == "Goblin Hunt");
    CHECK(qq->min_level == 1);
    CHECK(qq->xp_reward == 50);
    CHECK(qq->gold_reward == 100);
    CHECK(qq->loot_rarity == ItemRarity::Rare);
    CHECK(qq->active);

    // Bad quest inputs
    CHECK(!g.CreateQuest("", 1, 50, 100, ItemRarity::Rare, 5000, 0).has_value());
    CHECK(!g.CreateQuest("Q", 0, 50, 100, ItemRarity::Rare, 5000, 0).has_value());
    CHECK(!g.CreateQuest("Q", 1, 50, 100, ItemRarity::Rare, 20000, 0).has_value());
    CHECK(!g.CreateQuest("Q", 1, 50, 100, ItemRarity::Rare, 5000, -1).has_value());

    // High-level quest
    auto q2 = g.CreateQuest("Dragon Slayer", 10, 1000, 5000,
                            ItemRarity::Legendary, 10000, 0);
    CHECK(q2.has_value());

    // Alice (level 3) can't do dragon quest (min level 10)
    CHECK(!g.CompleteQuest("alice", *q2, false).has_value());

    // Alice completes goblin quest
    uint64_t before_xp = g.GetPlayer("alice")->xp;
    uint64_t before_gold = g.GetPlayer("alice")->gold;
    auto qr = g.CompleteQuest("alice", *q1, false);
    CHECK(qr.has_value());
    CHECK(qr->xp_gained == 50);
    CHECK(qr->gold_gained == 100);
    CHECK(g.GetPlayer("alice")->xp == before_xp + 50);
    CHECK(g.GetPlayer("alice")->gold == before_gold + 100);
    CHECK(g.GetPlayer("alice")->quests_completed == 1);

    // Unknown player / quest
    CHECK(!g.CompleteQuest("nope", *q1, false).has_value());
    CHECK(!g.CompleteQuest("alice", 99999, false).has_value());

    // Force loot (Rare)
    auto qr2 = g.CompleteQuest("bob", *q1, true);
    CHECK(qr2.has_value());
    CHECK(qr2->loot_dropped);
    CHECK(qr2->loot_item_id != 0);
    CHECK(g.GetItem(qr2->loot_item_id)->rarity == ItemRarity::Rare);
    CHECK(g.GetItem(qr2->loot_item_id)->owner == "bob");

    // Deactivate quest
    CHECK(g.SetQuestActive(*q1, false));
    CHECK(!g.GetQuest(*q1)->active);
    CHECK(!g.CompleteQuest("carol", *q1, false).has_value());
    CHECK(g.SetQuestActive(*q1, true));

    // Active quests list
    auto active = g.ActiveQuests();
    CHECK(active.size() == 2);

    // Cooldown test
    auto q3 = g.CreateQuest("Daily", 1, 10, 10, ItemRarity::Common, 0, 3600);
    CHECK(q3.has_value());
    CHECK(g.CompleteQuest("carol", *q3, false).has_value());
    // Second immediate attempt fails due to cooldown
    CHECK(!g.CompleteQuest("carol", *q3, false).has_value());

    // ---- PvP ----
    CHECK(g.RecordPvp("alice", "bob"));
    CHECK(g.GetPlayer("alice")->pvp_wins == 1);
    CHECK(g.GetPlayer("bob")->pvp_losses == 1);

    CHECK(!g.RecordPvp("alice", "alice"));
    CHECK(!g.RecordPvp("nope", "bob"));
    CHECK(!g.RecordPvp("alice", ""));
    CHECK(!g.RecordPvp("", "bob"));

    // ---- Leaderboard ----
    // alice xp = 350, bob xp = 50, carol xp = 10
    auto top = g.TopByXp(10);
    CHECK(top.size() == 3);
    CHECK(top[0].first == "alice");
    CHECK(top[1].first == "bob");
    CHECK(top[2].first == "carol");
    CHECK(top[0].second > top[1].second);

    auto top2 = g.TopByXp(2);
    CHECK(top2.size() == 2);

    // ---- Grant to max level behavior ----
    // Not testing huge XP — level cap is fine at 100.
    CHECK(g.GrantXp("carol", 1000000000000ULL));
    CHECK(g.GetPlayer("carol")->level == MAX_LEVEL);
    CHECK(g.XpToNextLevel("carol") == 0);

    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}