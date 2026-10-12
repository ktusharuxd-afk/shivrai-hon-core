#include "voting_escrow.h"
#include <ctime>
namespace shivrai::voting_escrow {
VotingEscrow::VotingEscrow() = default;
std::optional<LockId> VotingEscrow::Lock(const std::string& o, uint64_t a, int64_t d) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (o.empty() || a == 0 || d <= 0 || d > MAX_LOCK_SEC) return std::nullopt;
    LockId id = next_id_++;
    LockEntry L;
    L.id = id; L.owner = o; L.amount = a;
    L.created_at = (int64_t)std::time(nullptr);
    L.unlock_at = L.created_at + d;
    locks_[id] = L;
    by_owner_[o].push_back(id);
    return id;
}
bool VotingEscrow::IncreaseAmount(LockId id, const std::string& o, uint64_t a) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = locks_.find(id);
    if (it == locks_.end() || it->second.owner != o || a == 0) return false;
    it->second.amount += a; return true;
}
bool VotingEscrow::ExtendLock(LockId id, const std::string& o, int64_t d) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = locks_.find(id);
    if (it == locks_.end() || it->second.owner != o || d <= 0 || d > MAX_LOCK_SEC) return false;
    it->second.unlock_at = (int64_t)std::time(nullptr) + d;
    return true;
}
bool VotingEscrow::Withdraw(LockId id, const std::string& o) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = locks_.find(id);
    if (it == locks_.end() || it->second.owner != o) return false;
    int64_t now = (int64_t)std::time(nullptr);
    if (now < it->second.unlock_at) return false;
    auto& v = by_owner_[o];
    for (auto i = v.begin(); i != v.end(); ++i) if (*i == id) { v.erase(i); break; }
    locks_.erase(it);
    return true;
}
uint64_t VotingEscrow::VotingPower(LockId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = locks_.find(id);
    if (it == locks_.end()) return 0;
    int64_t now = (int64_t)std::time(nullptr);
    if (now >= it->second.unlock_at) return 0;
    int64_t remaining = it->second.unlock_at - now;
    return (uint64_t)((__uint128_t)it->second.amount * remaining / MAX_LOCK_SEC);
}
uint64_t VotingEscrow::TotalVotingPowerOf(const std::string& o) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = by_owner_.find(o);
    if (it == by_owner_.end()) return 0;
    uint64_t t = 0;
    for (auto id : it->second) t += VotingPower(id);
    return t;
}
uint64_t VotingEscrow::TotalLocked() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    uint64_t t = 0; for (auto& [k, v] : locks_) t += v.amount; return t;
}
uint64_t VotingEscrow::TotalVotingPower() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    uint64_t t = 0; for (auto& [k, v] : locks_) t += VotingPower(k); return t;
}
std::optional<LockEntry> VotingEscrow::Get(LockId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = locks_.find(id);
    if (it == locks_.end()) return std::nullopt;
    return it->second;
}
std::vector<LockId> VotingEscrow::LocksOf(const std::string& o) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = by_owner_.find(o);
    return it == by_owner_.end() ? std::vector<LockId>{} : it->second;
}
size_t VotingEscrow::LockCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return locks_.size();
}
}
