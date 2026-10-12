#include "shivrai/wallet/asset_registry.h"
#include <stdexcept>

namespace shivrai {

AssetRegistry::AssetRegistry() {
    AssetInfo native;
    native.asset_id = NativeId();
    native.kind = AssetKind::NATIVE;
    native.ticker = "HON";
    native.name = "Shivrai";
    native.decimals = 8;
    assets_[native.asset_id] = native;
}

const std::string& AssetRegistry::NativeId() {
    static const std::string id = std::string(64, '0');
    return id;
}

bool AssetRegistry::RegisterAsset(const AssetInfo& info) {
    std::lock_guard<std::recursive_mutex> lock(cs_);
    if (info.asset_id.empty() || info.ticker.empty()) return false;
    if (assets_.count(info.asset_id)) return false;
    assets_[info.asset_id] = info;
    return true;
}

std::optional<AssetInfo> AssetRegistry::GetAsset(const std::string& asset_id) const {
    std::lock_guard<std::recursive_mutex> lock(cs_);
    auto it = assets_.find(asset_id);
    if (it == assets_.end()) return std::nullopt;
    return it->second;
}

std::vector<AssetInfo> AssetRegistry::ListAssets() const {
    std::lock_guard<std::recursive_mutex> lock(cs_);
    std::vector<AssetInfo> out;
    for (const auto& kv : assets_) out.push_back(kv.second);
    return out;
}

void AssetRegistry::Credit(const std::string& address, const std::string& asset_id, uint64_t amount) {
    std::lock_guard<std::recursive_mutex> lock(cs_);
    balances_[{address, asset_id}] += amount;
}

bool AssetRegistry::Debit(const std::string& address, const std::string& asset_id, uint64_t amount) {
    std::lock_guard<std::recursive_mutex> lock(cs_);
    auto it = balances_.find({address, asset_id});
    if (it == balances_.end() || it->second < amount) return false;
    it->second -= amount;
    if (it->second == 0) balances_.erase(it);
    return true;
}

uint64_t AssetRegistry::BalanceOf(const std::string& address, const std::string& asset_id) const {
    std::lock_guard<std::recursive_mutex> lock(cs_);
    auto it = balances_.find({address, asset_id});
    return it == balances_.end() ? 0 : it->second;
}

std::vector<AssetBalance> AssetRegistry::BalancesOf(const std::string& address) const {
    std::lock_guard<std::recursive_mutex> lock(cs_);
    std::vector<AssetBalance> out;
    for (const auto& kv : balances_) {
        if (kv.first.first != address) continue;
        AssetBalance b;
        b.asset_id = kv.first.second;
        b.amount = kv.second;
        auto ai = assets_.find(b.asset_id);
        if (ai != assets_.end()) b.ticker = ai->second.ticker;
        out.push_back(b);
    }
    return out;
}

bool AssetRegistry::MintNft(const NftRecord& nft) {
    std::lock_guard<std::recursive_mutex> lock(cs_);
    auto key = std::make_pair(nft.asset_id, nft.token_id);
    if (nfts_.count(key)) return false;
    nfts_[key] = nft;
    return true;
}

bool AssetRegistry::TransferNft(const std::string& asset_id, uint32_t token_id, const std::string& to) {
    std::lock_guard<std::recursive_mutex> lock(cs_);
    auto it = nfts_.find({asset_id, token_id});
    if (it == nfts_.end()) return false;
    it->second.owner_address = to;
    return true;
}

std::optional<NftRecord> AssetRegistry::GetNft(const std::string& asset_id, uint32_t token_id) const {
    std::lock_guard<std::recursive_mutex> lock(cs_);
    auto it = nfts_.find({asset_id, token_id});
    if (it == nfts_.end()) return std::nullopt;
    return it->second;
}

std::vector<NftRecord> AssetRegistry::NftsOf(const std::string& address) const {
    std::lock_guard<std::recursive_mutex> lock(cs_);
    std::vector<NftRecord> out;
    for (const auto& kv : nfts_) if (kv.second.owner_address == address) out.push_back(kv.second);
    return out;
}

} // namespace shivrai
