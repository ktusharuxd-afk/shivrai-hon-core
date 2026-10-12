#ifndef SHIVRAI_WALLET_ASSET_REGISTRY_H
#define SHIVRAI_WALLET_ASSET_REGISTRY_H

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace shivrai {

enum class AssetKind { NATIVE, TOKEN, NFT };

/** Asset metadata. */
struct AssetInfo {
    std::string asset_id;       // hex 32 bytes
    AssetKind kind{AssetKind::TOKEN};
    std::string ticker;         // e.g. "HON"
    std::string name;
    uint8_t decimals{8};
    uint64_t max_supply{0};
};

/** Per-address balance of one asset. */
struct AssetBalance {
    std::string asset_id;
    uint64_t amount{0};          // smallest units
    std::string ticker;
};

/** NFT (unique, indivisible). */
struct NftRecord {
    std::string asset_id;       // collection id
    uint32_t token_id{0};
    std::string owner_address;
    std::string uri;
    std::map<std::string, std::string> metadata;
};

/** In-memory multi-asset registry + balances. */
class AssetRegistry {
public:
    AssetRegistry();

    /** Native HON asset id (0x00..00). */
    static const std::string& NativeId();

    // Registration
    bool RegisterAsset(const AssetInfo& info);
    std::optional<AssetInfo> GetAsset(const std::string& asset_id) const;
    std::vector<AssetInfo> ListAssets() const;

    // Balances
    void Credit(const std::string& address, const std::string& asset_id, uint64_t amount);
    bool Debit(const std::string& address, const std::string& asset_id, uint64_t amount);
    uint64_t BalanceOf(const std::string& address, const std::string& asset_id) const;
    std::vector<AssetBalance> BalancesOf(const std::string& address) const;

    // NFTs
    bool MintNft(const NftRecord& nft);
    bool TransferNft(const std::string& asset_id, uint32_t token_id, const std::string& to);
    std::optional<NftRecord> GetNft(const std::string& asset_id, uint32_t token_id) const;
    std::vector<NftRecord> NftsOf(const std::string& address) const;

private:
    mutable std::recursive_mutex cs_;
    std::map<std::string, AssetInfo> assets_;
    std::map<std::pair<std::string, std::string>, uint64_t> balances_; // (addr, asset) -> amount
    std::map<std::pair<std::string, uint32_t>, NftRecord> nfts_;
};

} // namespace shivrai

#endif
