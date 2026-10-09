#ifndef SHIVRAI_HON_GAMING_H
#define SHIVRAI_HON_GAMING_H
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace shivrai::gaming {

enum class AssetRarity { COMMON, UNCOMMON, RARE, EPIC, LEGENDARY };
enum class AssetStatus { ACTIVE, LOCKED, BURNED };

struct GameAsset {
    std::string id;
    std::string owner;
    std::string game_id;
    std::string asset_type;
    std::string name;
    AssetRarity rarity{AssetRarity::COMMON};
    AssetStatus status{AssetStatus::ACTIVE};
    uint64_t minted_at{0};
    std::string metadata_uri;
    uint32_t level{1};
    uint64_t experience{0};
};

struct GameReward {
    std::string player;
    std::string game_id;
    uint64_t amount{0};
    std::string currency;
    std::string reason;
    uint64_t timestamp{0};
};

class GameAssetRegistry {
public:
    std::string mint_asset(const std::string& owner, const std::string& game_id,
                            const std::string& asset_type, const std::string& name,
                            AssetRarity rarity, const std::string& metadata_uri = "");

    bool transfer_asset(const std::string& asset_id, const std::string& to);
    bool lock_asset(const std::string& asset_id);
    bool unlock_asset(const std::string& asset_id);
    bool burn_asset(const std::string& asset_id);
    bool level_up(const std::string& asset_id, uint64_t xp_gained);

    bool record_reward(const std::string& player, const std::string& game_id,
                        uint64_t amount, const std::string& currency,
                        const std::string& reason);

    const GameAsset* get_asset(const std::string& id) const;
    std::vector<GameAsset> assets_by_owner(const std::string& owner) const;
    std::vector<GameAsset> assets_by_game(const std::string& game_id) const;
    std::vector<GameReward> rewards_by_player(const std::string& player) const;

    size_t total_assets() const { return assets_.size(); }

private:
    std::unordered_map<std::string, GameAsset> assets_;
    std::vector<GameReward> rewards_;
    std::string gen_id() const;
};

} // namespace shivrai::gaming
#endif
