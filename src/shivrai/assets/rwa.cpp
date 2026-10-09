#include "rwa.h"
#include "shivrai/common/time.hpp"
#include <random>
#include <sstream>

namespace shivrai::assets {

std::string RWARegistry::gen_id() const {
    static std::mt19937_64 rng(shivrai::common::monotonic_ns());
    std::ostringstream ss;
    ss << "RWA_" << rng();
    return ss.str();
}

std::string RWARegistry::register_asset(const std::string& owner, RWAType type,
                                          const std::string& description, uint64_t value,
                                          const std::string& currency, const std::string& issuer,
                                          const std::string& jurisdiction,
                                          const std::string& legal_doc_hash) {
    if (owner.empty() || value == 0 || currency.empty()) return "";
    RWAAsset a;
    a.id = gen_id();
    a.owner = owner;
    a.type = type;
    a.description = description;
    a.value = value;
    a.currency = currency;
    a.issuer = issuer;
    a.jurisdiction = jurisdiction;
    a.legal_doc_hash = legal_doc_hash;
    a.issued_at = shivrai::common::now_seconds();
    a.status = RWAStatus::PENDING;
    assets_[a.id] = a;
    return a.id;
}

bool RWARegistry::activate(const std::string& id) {
    auto it = assets_.find(id);
    if (it == assets_.end() || it->second.status != RWAStatus::PENDING) return false;
    it->second.status = RWAStatus::ACTIVE;
    return true;
}

bool RWARegistry::freeze(const std::string& id) {
    auto it = assets_.find(id);
    if (it == assets_.end()) return false;
    it->second.status = RWAStatus::FROZEN;
    return true;
}

bool RWARegistry::redeem(const std::string& id) {
    auto it = assets_.find(id);
    if (it == assets_.end() || it->second.status != RWAStatus::ACTIVE) return false;
    it->second.status = RWAStatus::REDEEMED;
    return true;
}

bool RWARegistry::transfer(const std::string& asset_id, const std::string& to,
                             const std::string& tx_ref) {
    auto it = assets_.find(asset_id);
    if (it == assets_.end() || it->second.status != RWAStatus::ACTIVE) return false;
    RWATransfer t;
    t.asset_id = asset_id;
    t.from = it->second.owner;
    t.to = to;
    t.timestamp = shivrai::common::now_seconds();
    t.tx_ref = tx_ref;
    transfers_[asset_id].push_back(t);
    it->second.owner = to;
    return true;
}

const RWAAsset* RWARegistry::get(const std::string& id) const {
    auto it = assets_.find(id);
    return it == assets_.end() ? nullptr : &it->second;
}

std::vector<RWAAsset> RWARegistry::by_owner(const std::string& owner) const {
    std::vector<RWAAsset> result;
    for (const auto& [id, a] : assets_)
        if (a.owner == owner) result.push_back(a);
    return result;
}

std::vector<RWATransfer> RWARegistry::transfer_history(const std::string& asset_id) const {
    auto it = transfers_.find(asset_id);
    return it == transfers_.end() ? std::vector<RWATransfer>{} : it->second;
}

} // namespace shivrai::assets
