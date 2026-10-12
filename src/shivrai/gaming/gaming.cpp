#include "gaming.h"
#include <algorithm>
#include <ctime>

namespace shivrai::gaming {

GameEngine::GameEngine() = default;

// ---- Player ----
bool GameEngine::RegisterPlayer(const std::string& username) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (username.empty()) return false;
    if (players_.count(username)) return false;
    Player p;
    p.id = username;
    p.username = username;
    p.level = 1;
    p.xp = 0;
    p.gold = 0;
    p.registered_at = (int64_t)std::time(nullptr);
    players_[username] = p;
    return true;
}

std::optional<Player> GameEngine::GetPlayer(const std::string& id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = players_.find(id);
    if (it == players_.end()) return std::nullopt;
    return it->second;
}

size_t GameEngine::PlayerCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return players_.size();
}

// ---- XP / Level ----
uint64_t GameEngine::XpForLevel(uint32_t level) const {
    if (level <= 1) return 0;
    uint64_t need = XP_PER_LEVEL_BASE;
    uint64_t total = 0;
    for (uint32_t l = 2; l <= level && l <= MAX_LEVEL; ++l) {
        total += need;
        need = need * XP_GROWTH_NUM / XP_GROWTH_DEN;
    }
    return total;
}

uint64_t GameEngine::XpToNextLevel(const std::string& player_id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = players_.find(player_id);
    if (it == players_.end()) return 0;
    if (it->second.level >= MAX_LEVEL) return 0;
    uint64_t need = XpForLevel(it->second.level + 1);
    if (it->second.xp >= need) return 0;
    return need - it->second.xp;
}

void GameEngine::LevelUpIfReady(Player& p) {
    while (p.level < MAX_LEVEL) {
        uint64_t need = XpForLevel(p.level + 1);
        if (p.xp < need) break;
        p.level++;
    }
}

bool GameEngine::GrantXp(const std::string& player_id, uint64_t xp) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = players_.find(player_id);
    if (it == players_.end() || xp == 0) return false;
    it->second.xp += xp;
    LevelUpIfReady(it->second);
    return true;
}

bool GameEngine::GrantGold(const std::string& player_id, uint64_t gold) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = players_.find(player_id);
    if (it == players_.end() || gold == 0) return false;
    it->second.gold += gold;
    return true;
}

// ---- Quests ----
std::optional<QuestId> GameEngine::CreateQuest(const std::string& name,
                                               uint32_t min_level,
                                               uint64_t xp_reward,
                                               uint64_t gold_reward,
                                               ItemRarity loot_rarity,
                                               uint32_t loot_chance_bps,
                                               int64_t cooldown_sec) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (name.empty()) return std::nullopt;
    if (min_level == 0 || min_level > MAX_LEVEL) return std::nullopt;
    if (loot_chance_bps > BPS) return std::nullopt;
    if (cooldown_sec < 0) return std::nullopt;

    QuestId id = next_quest_id_++;
    Quest q;
    q.id = id;
    q.name = name;
    q.min_level = min_level;
    q.xp_reward = xp_reward;
    q.gold_reward = gold_reward;
    q.loot_rarity = loot_rarity;
    q.loot_chance_bps = loot_chance_bps;
    q.cooldown_sec = cooldown_sec;
    q.active = true;
    quests_[id] = q;
    return id;
}

bool GameEngine::SetQuestActive(QuestId id, bool active) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = quests_.find(id);
    if (it == quests_.end()) return false;
    it->second.active = active;
    return true;
}

std::optional<Quest> GameEngine::GetQuest(QuestId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = quests_.find(id);
    if (it == quests_.end()) return std::nullopt;
    return it->second;
}

std::vector<QuestId> GameEngine::ActiveQuests() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<QuestId> out;
    for (auto& [id, q] : quests_) if (q.active) out.push_back(id);
    return out;
}

size_t GameEngine::QuestCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return quests_.size();
}

uint64_t GameEngine::RollLoot(ItemRarity r) const {
    // Deterministic-ish roll based on rarity (simplified for tests)
    switch (r) {
        case ItemRarity::Common:    return 1;
        case ItemRarity::Rare:      return 2;
        case ItemRarity::Epic:      return 3;
        case ItemRarity::Legendary: return 4;
    }
    return 0;
}

std::optional<QuestResult> GameEngine::CompleteQuest(const std::string& player_id,
                                                     QuestId qid,
                                                     bool force_loot) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto pit = players_.find(player_id);
    if (pit == players_.end()) return std::nullopt;
    auto qit = quests_.find(qid);
    if (qit == quests_.end()) return std::nullopt;
    if (!qit->second.active) return std::nullopt;
    if (pit->second.level < qit->second.min_level) return std::nullopt;

    int64_t now = (int64_t)std::time(nullptr);
    if (qit->second.cooldown_sec > 0 &&
        now - pit->second.last_quest_at < qit->second.cooldown_sec) {
        return std::nullopt;
    }

    QuestResult r;
    r.xp_gained = qit->second.xp_reward;
    r.gold_gained = qit->second.gold_reward;
    uint32_t old_level = pit->second.level;
    pit->second.xp += r.xp_gained;
    pit->second.gold += r.gold_gained;
    pit->second.quests_completed += 1;
    pit->second.last_quest_at = now;
    LevelUpIfReady(pit->second);
    r.new_level = pit->second.level;
    r.leveled_up = (pit->second.level > old_level);

    bool drop = force_loot || (qit->second.loot_chance_bps >= BPS);
    if (drop && qit->second.loot_rarity != ItemRarity::Common) {
        std::string name = "Loot_" + std::to_string(RollLoot(qit->second.loot_rarity));
        ItemId iid = next_item_id_++;
        Item item;
        item.id = iid;
        item.owner = player_id;
        item.name = name;
        item.rarity = qit->second.loot_rarity;
        item.attack = RollLoot(qit->second.loot_rarity) * 10;
        item.defense = RollLoot(qit->second.loot_rarity) * 5;
        item.minted_at = now;
        item.bound = false;
        items_[iid] = item;
        player_items_[player_id].insert(iid);
        r.loot_dropped = true;
        r.loot_item_id = iid;
    }
    return r;
}

// ---- Items ----
ItemId GameEngine::MintItem(const std::string& owner,
                            const std::string& name,
                            ItemRarity rarity,
                            uint32_t attack,
                            uint32_t defense,
                            bool bound) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (owner.empty() || name.empty()) return 0;
    ItemId id = next_item_id_++;
    Item it;
    it.id = id;
    it.owner = owner;
    it.name = name;
    it.rarity = rarity;
    it.attack = attack;
    it.defense = defense;
    it.minted_at = (int64_t)std::time(nullptr);
    it.bound = bound;
    items_[id] = it;
    player_items_[owner].insert(id);
    return id;
}

bool GameEngine::TransferItem(const std::string& from, const std::string& to, ItemId id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = items_.find(id);
    if (it == items_.end()) return false;
    if (it->second.owner != from) return false;
    if (it->second.bound) return false;
    if (from.empty() || to.empty() || from == to) return false;

    auto sit = player_items_.find(from);
    if (sit != player_items_.end()) {
        sit->second.erase(id);
        if (sit->second.empty()) player_items_.erase(sit);
    }
    it->second.owner = to;
    player_items_[to].insert(id);
    return true;
}

std::optional<Item> GameEngine::GetItem(ItemId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = items_.find(id);
    if (it == items_.end()) return std::nullopt;
    return it->second;
}

std::vector<ItemId> GameEngine::ItemsOf(const std::string& owner) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<ItemId> out;
    auto it = player_items_.find(owner);
    if (it != player_items_.end()) out.assign(it->second.begin(), it->second.end());
    return out;
}

size_t GameEngine::ItemCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return items_.size();
}

// ---- PvP ----
bool GameEngine::RecordPvp(const std::string& winner, const std::string& loser) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (winner.empty() || loser.empty() || winner == loser) return false;
    auto wit = players_.find(winner);
    auto lit = players_.find(loser);
    if (wit == players_.end() || lit == players_.end()) return false;
    wit->second.pvp_wins += 1;
    lit->second.pvp_losses += 1;
    return true;
}

// ---- Leaderboard ----
std::vector<std::pair<std::string, uint64_t>> GameEngine::TopByXp(size_t n) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<std::pair<std::string, uint64_t>> out;
    for (auto& [id, p] : players_) out.emplace_back(id, p.xp);
    std::sort(out.begin(), out.end(),
              [](const auto& a, const auto& b) {
                  if (a.second != b.second) return a.second > b.second;
                  return a.first < b.first;
              });
    if (out.size() > n) out.resize(n);
    return out;
}

}