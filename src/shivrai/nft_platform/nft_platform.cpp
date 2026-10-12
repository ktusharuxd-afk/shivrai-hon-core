#include "nft_platform.h"

namespace shivrai::nft_platform {

NFTPlatform::NFTPlatform() = default;

std::optional<CollectionId> NFTPlatform::CreateCollection(
        const std::string& creator,
        const std::string& name,
        const std::string& symbol,
        uint32_t royalty_bps,
        uint64_t max_supply,
        const std::string& base_uri) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (creator.empty() || name.empty() || symbol.empty()) return std::nullopt;
    if (royalty_bps > MAX_ROYALTY_BPS) return std::nullopt;

    CollectionId id = next_cid_++;
    Collection c;
    c.id = id;
    c.creator = creator;
    c.name = name;
    c.symbol = symbol;
    c.royalty_bps = royalty_bps;
    c.max_supply = max_supply;
    c.base_uri = base_uri;
    collections_[id] = c;
    return id;
}

std::optional<Collection> NFTPlatform::GetCollection(CollectionId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = collections_.find(id);
    if (it == collections_.end()) return std::nullopt;
    return it->second;
}

size_t NFTPlatform::CollectionCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return collections_.size();
}

std::optional<GlobalId> NFTPlatform::Mint(CollectionId cid,
                                          const std::string& to,
                                          const std::string& token_uri) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto cit = collections_.find(cid);
    if (cit == collections_.end()) return std::nullopt;
    if (to.empty()) return std::nullopt;
    if (cit->second.max_supply != 0 && cit->second.minted >= cit->second.max_supply)
        return std::nullopt;

    GlobalId gid = next_gid_++;
    TokenId  tid = next_token_id_[cid]++;

    Token t;
    t.gid = gid;
    t.collection = cid;
    t.token_id = tid;
    t.owner = to;
    t.token_uri = token_uri;

    tokens_[gid] = t;
    collection_tokens_[cid].insert(gid);
    owner_tokens_[to].insert(gid);
    cit->second.minted++;
    return gid;
}

void NFTPlatform::RemoveFromOwnerIndex(const std::string& owner, GlobalId gid) {
    auto it = owner_tokens_.find(owner);
    if (it == owner_tokens_.end()) return;
    it->second.erase(gid);
    if (it->second.empty()) owner_tokens_.erase(it);
}

bool NFTPlatform::Transfer(const std::string& from,
                           const std::string& to,
                           GlobalId gid) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (from.empty() || to.empty() || from == to) return false;
    auto it = tokens_.find(gid);
    if (it == tokens_.end()) return false;
    if (it->second.owner != from) return false;

    RemoveFromOwnerIndex(from, gid);
    it->second.owner = to;
    owner_tokens_[to].insert(gid);
    approvals_.erase(gid);
    return true;
}

bool NFTPlatform::Approve(const std::string& owner,
                          const std::string& approved,
                          GlobalId gid) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = tokens_.find(gid);
    if (it == tokens_.end()) return false;
    if (it->second.owner != owner) return false;
    if (approved == owner) return false;
    approvals_[gid] = approved;
    return true;
}

bool NFTPlatform::SetApprovalForAll(const std::string& owner,
                                    const std::string& oper,
                                    bool approved) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (owner.empty() || oper.empty() || owner == oper) return false;
    if (approved) {
        operator_approvals_[owner].insert(oper);
    } else {
        auto it = operator_approvals_.find(owner);
        if (it != operator_approvals_.end()) {
            it->second.erase(oper);
            if (it->second.empty()) operator_approvals_.erase(it);
        }
    }
    return true;
}

bool NFTPlatform::TransferFrom(const std::string& spender,
                               const std::string& from,
                               const std::string& to,
                               GlobalId gid) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = tokens_.find(gid);
    if (it == tokens_.end()) return false;
    if (it->second.owner != from) return false;
    if (spender == from) return Transfer(from, to, gid);

    bool allowed = false;
    auto ait = approvals_.find(gid);
    if (ait != approvals_.end() && ait->second == spender) allowed = true;
    auto oit = operator_approvals_.find(from);
    if (oit != operator_approvals_.end() && oit->second.count(spender)) allowed = true;
    if (!allowed) return false;

    RemoveFromOwnerIndex(from, gid);
    it->second.owner = to;
    owner_tokens_[to].insert(gid);
    approvals_.erase(gid);
    return true;
}

bool NFTPlatform::Burn(const std::string& owner, GlobalId gid) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = tokens_.find(gid);
    if (it == tokens_.end()) return false;
    if (it->second.owner != owner) return false;
    RemoveFromOwnerIndex(owner, gid);
    collection_tokens_[it->second.collection].erase(gid);
    auto cit = collections_.find(it->second.collection);
    if (cit != collections_.end() && cit->second.minted > 0) cit->second.minted--;
    approvals_.erase(gid);
    tokens_.erase(it);
    return true;
}

std::optional<Token> NFTPlatform::GetToken(GlobalId gid) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = tokens_.find(gid);
    if (it == tokens_.end()) return std::nullopt;
    return it->second;
}

std::string NFTPlatform::OwnerOf(GlobalId gid) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = tokens_.find(gid);
    return it == tokens_.end() ? std::string() : it->second.owner;
}

std::vector<GlobalId> NFTPlatform::TokensOf(const std::string& owner) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<GlobalId> out;
    auto it = owner_tokens_.find(owner);
    if (it != owner_tokens_.end()) out.assign(it->second.begin(), it->second.end());
    return out;
}

std::vector<GlobalId> NFTPlatform::TokensInCollection(CollectionId cid) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<GlobalId> out;
    auto it = collection_tokens_.find(cid);
    if (it != collection_tokens_.end()) out.assign(it->second.begin(), it->second.end());
    return out;
}

uint64_t NFTPlatform::TotalSupply(CollectionId cid) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = collections_.find(cid);
    return it == collections_.end() ? 0 : it->second.minted;
}

size_t NFTPlatform::TotalTokens() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return tokens_.size();
}

std::optional<std::string> NFTPlatform::GetApproved(GlobalId gid) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = approvals_.find(gid);
    if (it == approvals_.end()) return std::nullopt;
    return it->second;
}

bool NFTPlatform::IsApprovedForAll(const std::string& owner,
                                   const std::string& oper) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = operator_approvals_.find(owner);
    return it != operator_approvals_.end() && it->second.count(oper) > 0;
}

std::optional<SaleResult> NFTPlatform::RecordSale(GlobalId gid, uint64_t sale_price) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = tokens_.find(gid);
    if (it == tokens_.end()) return std::nullopt;
    auto cit = collections_.find(it->second.collection);
    if (cit == collections_.end()) return std::nullopt;

    SaleResult r;
    r.royalty_recipient = cit->second.creator;
    r.royalty_paid = (uint64_t)((__uint128_t)sale_price * cit->second.royalty_bps / 10000);
    r.seller_proceeds = sale_price - r.royalty_paid;
    return r;
}

}