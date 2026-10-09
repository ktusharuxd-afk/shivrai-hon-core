#include "assets.h"
#include "shivrai/common/time.hpp"
#include <random>
#include <sstream>

namespace shivrai::gaming {

std::string GameAssetRegistry::gen_id() const {
    static std::mt19937_64 rng(shivrai::common::monotonic_ns());
    std::ostringstream ss;
    ss << "GASSET_" << rng();
    return ss.str();
}

std::string GameAssetRegistry::mint_asset(const std::string& owner, const std::string& game_id,
                                           const std::string& asset_type, const std::string& name,
                                           AssetRarity rarity, const std::string& metadata_uri) {
    if (owner.empty() || game_id.empty() || name.empty()) return "";
    GameAsset a;
    a.id = gen_id();
    a.owner = owner;
    a.game_id = game_id;
    a.asset_type = asset_type;
    a.name = name;
    a.rarity = rarity;
    a.metadata_uri = metadata_uri;
    a.minted_at = shivrai::common::now_seconds();
    a.status = AssetStatus::ACTIVE;
    assets_[a.id] = a;
    return a.id;
}

bool GameAssetRegistry::transfer_asset(const std::string& id, const std::string& to) {
    auto it = assets_.find(id);
    if (it == assets_.end() || it->second.status != AssetStatus::ACTIVE) return false;
    it->second.owner = to;
    return true;
}

bool GameAssetRegistry::lock_asset(const std::string& id) {
    auto it = assets_.find(id);
    if (it == assets_.end() || it->second.status != AssetStatus::ACTIVE) return false;
    it->second.status = AssetStatus::LOCKED;
    return true;
}

bool GameAssetRegistry::unlock_asset(const std::string& id) {
    auto it = assets_.find(id);
    if (it == assets_.end() || it->second.status != AssetStatus::LOCKED) return false;
    it->second.status = AssetStatus::ACTIVE;
    return true;
}

bool GameAssetRegistry::burn_asset(const std::string& id) {
    auto it = assets_.find(id);
    if (it == assets_.end()) return false;
    it->second.status = AssetStatus::BURNED;
    return true;
}

bool GameAssetRegistry::level_up(const std::string& id, uint64_t xp_gained) {
    auto it = assets_.find(id);
    if (it == assets_.end() || it->second.status != AssetStatus::ACTIVE) return false;
    it->second.experience += xp_gained;
    it->second.level = (uint32_t)(it->second.experience / 1000) + 1;
    return true;
}

bool GameAssetRegistry::record_reward(const std::string& player, const std::string& game_id,
                                       uint64_t amount, const std::string& currency,
                                       const std::string& reason) {
    GameReward r;
    r.player = player;
    r.game_id = game_id;
    r.amount = amount;
    r.currency = currency;
    r.reason = reason;
    r.timestamp = shivrai::common::now_seconds();
    rewards_.push_back(r);
    return true;
}

const GameAsset* GameAssetRegistry::get_asset(const std::string& id) const {
    auto it = assets_.find(id);
    return it == assets_.end() ? nullptr : &it->second;
}

std::vector<GameAsset> GameAssetRegistry::assets_by_owner(const std::string& owner) const {
    std::vector<GameAsset> result;
    for (const auto& [id, a] : assets_)
        if (a.owner == owner) result.push_back(a);
    return result;
}

std::vector<GameAsset> GameAssetRegistry::assets_by_game(const std::string& game_id) const {
    std::vector<GameAsset> result;
    for (const auto& [id, a] : assets_)
        if (a.game_id == game_id) result.push_back(a);
    return result;
}

std::vector<GameReward> GameAssetRegistry::rewards_by_player(const std::string& player) const {
    std::vector<GameReward> result;
    for (const auto& r : rewards_)
        if (r.player == player) result.push_back(r);
    return result;
}

} // namespace shivrai::gaming
