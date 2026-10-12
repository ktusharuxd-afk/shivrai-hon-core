#include "kyc_registry.h"
#include <ctime>
namespace shivrai::kyc_registry {
KycRegistry::KycRegistry() = default;
bool KycRegistry::Register(const std::string& u, KycLevel l, const std::string& j,
                           const std::string& v, int64_t val) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (u.empty() || j.empty() || v.empty() || val <= 0) return false;
    if (records_.count(u)) return false;
    KycRecord r;
    r.user = u; r.level = l; r.jurisdiction = j;
    r.verifier = v;
    r.verified_at = (int64_t)std::time(nullptr);
    r.expires_at = r.verified_at + val;
    records_[u] = r;
    return true;
}
bool KycRegistry::Upgrade(const std::string& u, KycLevel nl, const std::string& v) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = records_.find(u);
    if (it == records_.end() || it->second.revoked) return false;
    if (v.empty()) return false;
    if ((int)nl <= (int)it->second.level) return false;
    it->second.level = nl;
    it->second.verifier = v;
    it->second.verified_at = (int64_t)std::time(nullptr);
    return true;
}
bool KycRegistry::Revoke(const std::string& u, const std::string& v) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = records_.find(u);
    if (it == records_.end() || it->second.revoked) return false;
    if (v.empty()) return false;
    it->second.revoked = true;
    return true;
}
std::optional<KycRecord> KycRegistry::Get(const std::string& u) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = records_.find(u);
    if (it == records_.end()) return std::nullopt;
    return it->second;
}
bool KycRegistry::IsValid(const std::string& u, KycLevel ml) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = records_.find(u);
    if (it == records_.end() || it->second.revoked) return false;
    int64_t now = (int64_t)std::time(nullptr);
    if (now >= it->second.expires_at) return false;
    return (int)it->second.level >= (int)ml;
}
KycLevel KycRegistry::LevelOf(const std::string& u) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = records_.find(u);
    if (it == records_.end() || it->second.revoked) return KycLevel::None;
    return it->second.level;
}
bool KycRegistry::SetJurisdiction(const std::string& u, const std::string& j) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = records_.find(u);
    if (it == records_.end() || j.empty()) return false;
    it->second.jurisdiction = j;
    return true;
}
size_t KycRegistry::Count() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return records_.size();
}
size_t KycRegistry::ValidCount(KycLevel ml) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    size_t n = 0;
    for (auto& [k, v] : records_) if (IsValid(k, ml)) ++n;
    return n;
}
}
