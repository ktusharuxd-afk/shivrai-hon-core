#ifndef SHIVRAI_HON_GAMING_H
#define SHIVRAI_HON_GAMING_H

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace shivrai::gaming {

using PlayerId  = std::string;
using ItemId    = uint64_t;    // global item id (bound to NFT)
using QuestId   = uint64_t;

static constexpr uint32_t BPS = 10000;
static constexpr uint32_t MAX_LEVEL = 100;
static constexpr uint64_t XP_PER_LEVEL_BASE = 100;
static constexpr uint64_t XP_GROWTH_NUM = 120;   // 1.2x per level
static constexpr uint64_t XP_GROWTH_DEN = 100;

enum class ItemRarity { Common = 0, Rare = 1, Epic = 2, Legendary = 3 };

struct Player {
    PlayerId    id;
    std::string username;
    uint32_t    level{1};
    uint64_t    xp{0};
    uint64_t    gold{0};
    int64_t     registered_at{0};
    int64_t     last_quest_at{0};
    uint32_t    quests_completed{0};
    uint32_t    pvp_wins{0};
    uint32_t    pvp_losses{0};
};

struct Item {
    ItemId      id{0};
    std::string owner;
    std::string name;
    ItemRarity  rarity{ItemRarity::Common};
    uint32_t    attack{0};
    uint32_t    defense{0};
    int64_t     minted_at{0};
    bool        bound{false};      // soulbound if true
};

struct Quest {
    QuestId     id{0};
    std::string name;
    uint32_t    min_level{1};
    uint64_t    xp_reward{0};
    uint64_t    gold_reward{0};
    ItemRarity  loot_rarity{ItemRarity::Common};
    uint32_t    loot_chance_bps{5000};   // 50%
    int64_t     cooldown_sec{0};
    bool        active{true};
};

struct QuestResult {
    uint64_t xp_gained{0};
    uint64_t gold_gained{0};
    bool     loot_dropped{false};
    ItemId   loot_item_id{0};
    uint32_t new_level{1};
    bool     leveled_up{false};
};

class GameEngine {
public:
    GameEngine();

    // --- Player ---
    bool RegisterPlayer(const std::string& username);
    std::optional<Player> GetPlayer(const std::string& id) const;
    size_t PlayerCount() const;

    // --- Quests ---
    std::optional<QuestId> CreateQuest(const std::string& name,
                                       uint32_t min_level,
                                       uint64_t xp_reward,
                                       uint64_t gold_reward,
                                       ItemRarity loot_rarity,
                                       uint32_t loot_chance_bps,
                                       int64_t cooldown_sec);
    bool SetQuestActive(QuestId id, bool active);
    std::optional<Quest> GetQuest(QuestId id) const;
    std::vector<QuestId> ActiveQuests() const;
    size_t QuestCount() const;

    std::optional<QuestResult> CompleteQuest(const std::string& player_id,
                                             QuestId qid,
                                             bool force_loot);

    // --- XP & Level ---
    uint64_t XpForLevel(uint32_t level) const;
    uint64_t XpToNextLevel(const std::string& player_id) const;
    bool     GrantXp(const std::string& player_id, uint64_t xp);
    bool     GrantGold(const std::string& player_id, uint64_t gold);

    // --- Items ---
    ItemId MintItem(const std::string& owner,
                    const std::string& name,
                    ItemRarity rarity,
                    uint32_t attack,
                    uint32_t defense,
                    bool bound);
    bool TransferItem(const std::string& from, const std::string& to, ItemId id);
    std::optional<Item> GetItem(ItemId id) const;
    std::vector<ItemId> ItemsOf(const std::string& owner) const;
    size_t ItemCount() const;

    // --- PvP ---
    bool RecordPvp(const std::string& winner, const std::string& loser);

    // --- Leaderboard ---
    std::vector<std::pair<std::string, uint64_t>> TopByXp(size_t n) const;

private:
    void LevelUpIfReady(Player& p);
    uint64_t RollLoot(ItemRarity r) const;

    mutable std::recursive_mutex cs_;
    std::map<std::string, Player> players_;
    std::map<QuestId, Quest>      quests_;
    std::map<ItemId, Item>        items_;
    std::map<std::string, std::set<ItemId>> player_items_;

    QuestId next_quest_id_{1};
    ItemId  next_item_id_{1};
};

}
#endif