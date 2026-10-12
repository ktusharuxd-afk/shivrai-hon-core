#include "upgradeable_proxy.h"
#include <ctime>
namespace shivrai::upgradeable_proxy {
UpgradeableProxy::UpgradeableProxy() = default;
bool UpgradeableProxy::Initialize(const std::string& a, const std::string& h) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (initialized_ || a.empty() || h.empty()) return false;
    admin_ = a; current_hash_ = h;
    Version v;
    v.impl_hash = h;
    v.deployed_at = (int64_t)std::time(nullptr);
    history_.push_back(v);
    initialized_ = true;
    return true;
}
bool UpgradeableProxy::Upgrade(const std::string& c, const std::string& nh,
                               uint32_t mj, uint32_t mn, uint32_t pt) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (!initialized_ || c != admin_ || nh.empty()) return false;
    Version v;
    v.major = mj; v.minor = mn; v.patch = pt;
    v.impl_hash = nh;
    v.deployed_at = (int64_t)std::time(nullptr);
    history_.push_back(v);
    current_hash_ = nh;
    return true;
}
bool UpgradeableProxy::Rollback(const std::string& c) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (!initialized_ || c != admin_) return false;
    if (history_.size() < 2) return false;
    history_.pop_back();
    current_hash_ = history_.back().impl_hash;
    return true;
}
bool UpgradeableProxy::TransferAdmin(const std::string& c, const std::string& na) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (!initialized_ || c != admin_ || na.empty() || na == admin_) return false;
    admin_ = na;
    return true;
}
std::string UpgradeableProxy::CurrentHash() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return current_hash_;
}
std::string UpgradeableProxy::Admin() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return admin_;
}
size_t UpgradeableProxy::VersionCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return history_.size();
}
std::optional<Version> UpgradeableProxy::VersionAt(size_t i) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (i >= history_.size()) return std::nullopt;
    return history_[i];
}
bool UpgradeableProxy::IsInitialized() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return initialized_;
}
}
