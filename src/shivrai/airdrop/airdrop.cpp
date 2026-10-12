#include "airdrop.h"
#include <ctime>
namespace shivrai::airdrop {
Airdrop::Airdrop() = default;
std::optional<RoundId> Airdrop::CreateRound(const std::string& n, int64_t s, int64_t e) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (n.empty() || e <= s) return std::nullopt;
    RoundId id = next_id_++;
    AirdropRound r;
    r.id = id; r.name = n; r.starts_at = s; r.ends_at = e;
    rounds_[id] = r;
    return id;
}
bool Airdrop::SetEligibility(RoundId id, const std::string& u, uint64_t a) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = rounds_.find(id);
    if (it == rounds_.end() || !it->second.active) return false;
    if (u.empty() || a == 0 || eligible_[id].count(u)) return false;
    eligible_[id][u] = a;
    it->second.total_allocated += a;
    return true;
}
std::optional<uint64_t> Airdrop::Claim(RoundId id, const std::string& u) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = rounds_.find(id);
    if (it == rounds_.end() || !it->second.active) return std::nullopt;
    auto eit = eligible_[id].find(u);
    if (eit == eligible_[id].end()) return std::nullopt;
    if (claimed_[id][u]) return std::nullopt;
    int64_t now = (int64_t)std::time(nullptr);
    if (now < it->second.starts_at || now > it->second.ends_at) return std::nullopt;
    claimed_[id][u] = true;
    it->second.total_claimed += eit->second;
    return eit->second;
}
std::optional<uint64_t> Airdrop::AmountFor(RoundId id, const std::string& u) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = eligible_.find(id);
    if (it == eligible_.end()) return std::nullopt;
    auto uit = it->second.find(u);
    if (uit == it->second.end()) return std::nullopt;
    return uit->second;
}
bool Airdrop::HasClaimed(RoundId id, const std::string& u) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = claimed_.find(id);
    if (it == claimed_.end()) return false;
    auto uit = it->second.find(u);
    return uit != it->second.end() && uit->second;
}
std::optional<AirdropRound> Airdrop::GetRound(RoundId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = rounds_.find(id);
    if (it == rounds_.end()) return std::nullopt;
    return it->second;
}
size_t Airdrop::RoundCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return rounds_.size();
}
}
