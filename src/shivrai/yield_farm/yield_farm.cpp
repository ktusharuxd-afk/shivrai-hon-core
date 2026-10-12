#include "yield_farm.h"
#include <ctime>
namespace shivrai::yield_farm {
YieldFarm::YieldFarm() = default;
std::optional<PoolId> YieldFarm::AddPool(const std::string& lp, uint64_t ap) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (lp.empty() || ap == 0 || by_lp_.count(lp)) return std::nullopt;
    PoolId id = next_id_++;
    FarmPool p; p.id = id; p.lp_token = lp; p.alloc_point = ap;
    p.last_reward_ts = (uint64_t)std::time(nullptr);
    pools_[id] = p;
    by_lp_[lp] = id;
    return id;
}
bool YieldFarm::SetRewardRate(uint64_t r) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    reward_rate_ = r; return true;
}
uint64_t YieldFarm::RewardRate() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return reward_rate_;
}
void YieldFarm::UpdatePool(PoolId id) {
    auto it = pools_.find(id);
    if (it == pools_.end()) return;
    FarmPool& p = it->second;
    uint64_t now = (uint64_t)std::time(nullptr);
    if (now <= p.last_reward_ts) return;
    if (p.total_staked == 0) { p.last_reward_ts = now; return; }
    uint64_t elapsed = now - p.last_reward_ts;
    __uint128_t reward = (__uint128_t)reward_rate_ * elapsed;
    uint64_t inc = (uint64_t)(reward * WAD / p.total_staked);
    p.acc_reward_per_share += inc;
    p.last_reward_ts = now;
}
bool YieldFarm::Stake(PoolId id, const std::string& user, uint64_t amount) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = pools_.find(id);
    if (it == pools_.end() || user.empty() || amount == 0) return false;
    UpdatePool(id);
    FarmPool& p = it->second;
    UserInfo& u = users_[id][user];
    if (u.amount > 0) {
        __uint128_t acc = (__uint128_t)u.amount * p.acc_reward_per_share / WAD;
        uint64_t acc64 = (uint64_t)acc;
        if (acc64 > u.reward_debt) u.pending += acc64 - u.reward_debt;
    }
    u.amount += amount;
    p.total_staked += amount;
    u.reward_debt = (uint64_t)((__uint128_t)u.amount * p.acc_reward_per_share / WAD);
    return true;
}
std::optional<uint64_t> YieldFarm::Withdraw(PoolId id, const std::string& user, uint64_t amount) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = pools_.find(id);
    if (it == pools_.end() || amount == 0) return std::nullopt;
    auto uit = users_[id].find(user);
    if (uit == users_[id].end() || uit->second.amount < amount) return std::nullopt;
    UpdatePool(id);
    FarmPool& p = it->second;
    UserInfo& u = uit->second;
    __uint128_t acc = (__uint128_t)u.amount * p.acc_reward_per_share / WAD;
    uint64_t acc64 = (uint64_t)acc;
    if (acc64 > u.reward_debt) u.pending += acc64 - u.reward_debt;
    u.amount -= amount;
    p.total_staked -= amount;
    u.reward_debt = (uint64_t)((__uint128_t)u.amount * p.acc_reward_per_share / WAD);
    return amount;
}
uint64_t YieldFarm::Pending(PoolId id, const std::string& user) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = pools_.find(id);
    if (it == pools_.end()) return 0;
    auto uit = users_.find(id);
    if (uit == users_.end()) return 0;
    auto uuit = uit->second.find(user);
    if (uuit == uit->second.end()) return 0;
    uint64_t now = (uint64_t)std::time(nullptr);
    uint64_t apr = it->second.acc_reward_per_share;
    if (it->second.total_staked > 0 && now > it->second.last_reward_ts) {
        uint64_t elapsed = now - it->second.last_reward_ts;
        __uint128_t reward = (__uint128_t)reward_rate_ * elapsed;
        apr += (uint64_t)(reward * WAD / it->second.total_staked);
    }
    __uint128_t acc = (__uint128_t)uuit->second.amount * apr / WAD;
    uint64_t acc64 = (uint64_t)acc;
    uint64_t pend = uuit->second.pending;
    if (acc64 > uuit->second.reward_debt) pend += acc64 - uuit->second.reward_debt;
    return pend;
}
std::optional<uint64_t> YieldFarm::Harvest(PoolId id, const std::string& user) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = pools_.find(id);
    if (it == pools_.end()) return std::nullopt;
    uint64_t p = Pending(id, user);
    auto uit = users_[id].find(user);
    if (uit == users_[id].end()) return std::nullopt;
    uit->second.pending = 0;
    UpdatePool(id);
    uit->second.reward_debt = (uint64_t)((__uint128_t)uit->second.amount * it->second.acc_reward_per_share / WAD);
    return p;
}
std::optional<FarmPool> YieldFarm::GetPool(PoolId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = pools_.find(id);
    if (it == pools_.end()) return std::nullopt;
    return it->second;
}
uint64_t YieldFarm::StakedOf(PoolId id, const std::string& user) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = users_.find(id);
    if (it == users_.end()) return 0;
    auto uit = it->second.find(user);
    return uit == it->second.end() ? 0 : uit->second.amount;
}
size_t YieldFarm::PoolCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return pools_.size();
}
}
