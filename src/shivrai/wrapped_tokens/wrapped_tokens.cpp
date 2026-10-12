#include "wrapped_tokens.h"
namespace shivrai::wrapped_tokens {
WrappedTokens::WrappedTokens() = default;
bool WrappedTokens::Register(const std::string& s, const std::string& c, const std::string& u) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (s.empty() || c.empty() || u.empty()) return false;
    if (assets_.count(s)) return false;
    WrappedAsset a;
    a.symbol = s; a.underlying_chain = c; a.underlying_symbol = u;
    assets_[s] = a;
    return true;
}
bool WrappedTokens::SetActive(const std::string& s, bool act) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = assets_.find(s);
    if (it == assets_.end()) return false;
    it->second.active = act;
    return true;
}
std::optional<WrappedAsset> WrappedTokens::Get(const std::string& s) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = assets_.find(s);
    if (it == assets_.end()) return std::nullopt;
    return it->second;
}
bool WrappedTokens::Wrap(const std::string& s, const std::string& u, uint64_t amt) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = assets_.find(s);
    if (it == assets_.end() || !it->second.active) return false;
    if (u.empty() || amt == 0) return false;
    it->second.total_supply += amt;
    it->second.total_backing += amt;
    balances_[s][u] += amt;
    return true;
}
std::optional<uint64_t> WrappedTokens::Unwrap(const std::string& s, const std::string& u, uint64_t amt) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = assets_.find(s);
    if (it == assets_.end()) return std::nullopt;
    auto bit = balances_[s].find(u);
    if (bit == balances_[s].end() || bit->second < amt) return std::nullopt;
    if (amt == 0) return std::nullopt;
    bit->second -= amt;
    it->second.total_supply -= amt;
    it->second.total_backing -= amt;
    return amt;
}
uint64_t WrappedTokens::BalanceOf(const std::string& s, const std::string& u) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = balances_.find(s);
    if (it == balances_.end()) return 0;
    auto uit = it->second.find(u);
    return uit == it->second.end() ? 0 : uit->second;
}
uint64_t WrappedTokens::TotalSupply(const std::string& s) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = assets_.find(s);
    return it == assets_.end() ? 0 : it->second.total_supply;
}
uint64_t WrappedTokens::TotalBacking(const std::string& s) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = assets_.find(s);
    return it == assets_.end() ? 0 : it->second.total_backing;
}
bool WrappedTokens::IsFullyBacked(const std::string& s) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = assets_.find(s);
    if (it == assets_.end()) return false;
    return it->second.total_backing >= it->second.total_supply;
}
size_t WrappedTokens::AssetCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return assets_.size();
}
}
