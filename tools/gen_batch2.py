#!/usr/bin/env python3
"""Batch 2: yield_farm, voting_escrow, atomic_swap, lottery, fee_market"""
import os, subprocess, sys
ROOT = os.path.expanduser("~/shivrai-hon-core")

def write(path, content):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f: f.write(content)

MODULES = {}

# ==================== yield_farm ====================
MODULES["yield_farm"] = {
"h": r'''#ifndef SHIVRAI_HON_YIELD_FARM_H
#define SHIVRAI_HON_YIELD_FARM_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
namespace shivrai::yield_farm {
using PoolId = uint64_t;
static constexpr uint64_t WAD = 1'000'000'000'000'000'000ULL;
struct FarmPool {
    PoolId id{0}; std::string lp_token;
    uint64_t alloc_point{0};
    uint64_t total_staked{0};
    uint64_t acc_reward_per_share{0};
    uint64_t last_reward_ts{0};
    uint64_t reward_rate{0};
};
struct UserInfo {
    uint64_t amount{0}; uint64_t reward_debt{0}; uint64_t pending{0};
};
class YieldFarm {
public:
    YieldFarm();
    std::optional<PoolId> AddPool(const std::string& lp_token, uint64_t alloc_point);
    bool SetRewardRate(uint64_t rate);
    uint64_t RewardRate() const;
    void UpdatePool(PoolId id);
    bool Stake(PoolId id, const std::string& user, uint64_t amount);
    std::optional<uint64_t> Withdraw(PoolId id, const std::string& user, uint64_t amount);
    uint64_t Pending(PoolId id, const std::string& user) const;
    std::optional<uint64_t> Harvest(PoolId id, const std::string& user);
    std::optional<FarmPool> GetPool(PoolId id) const;
    uint64_t StakedOf(PoolId id, const std::string& user) const;
    size_t PoolCount() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<PoolId, FarmPool> pools_;
    std::map<std::string, PoolId> by_lp_;
    std::map<PoolId, std::map<std::string, UserInfo>> users_;
    PoolId next_id_{1};
    uint64_t reward_rate_{0};
};
}
#endif
''',
"cpp": r'''#include "yield_farm.h"
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
''',
"test": r'''#include "shivrai/yield_farm/yield_farm.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::yield_farm;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    YieldFarm f;
    CHECK(f.PoolCount() == 0);
    CHECK(f.RewardRate() == 0);
    CHECK(!f.AddPool("", 100).has_value());
    CHECK(!f.AddPool("LP1", 0).has_value());
    auto p1 = f.AddPool("LP1", 100);
    auto p2 = f.AddPool("LP2", 200);
    CHECK(p1.has_value() && p2.has_value());
    CHECK(f.PoolCount() == 2);
    CHECK(!f.AddPool("LP1", 100).has_value());
    CHECK(f.GetPool(*p1)->lp_token == "LP1");
    CHECK(f.GetPool(*p1)->alloc_point == 100);
    CHECK(f.SetRewardRate(1000));
    CHECK(f.RewardRate() == 1000);
    CHECK(f.Stake(*p1, "alice", 5000));
    CHECK(f.StakedOf(*p1, "alice") == 5000);
    CHECK(f.GetPool(*p1)->total_staked == 5000);
    CHECK(!f.Stake(*p1, "", 100));
    CHECK(!f.Stake(*p1, "bob", 0));
    CHECK(!f.Stake(99999, "alice", 100));
    CHECK(f.Stake(*p1, "bob", 3000));
    CHECK(f.GetPool(*p1)->total_staked == 8000);
    CHECK(f.Pending(*p1, "alice") == 0);
    auto w = f.Withdraw(*p1, "alice", 2000);
    CHECK(w.has_value() && *w == 2000);
    CHECK(f.StakedOf(*p1, "alice") == 3000);
    CHECK(!f.Withdraw(*p1, "alice", 99999).has_value());
    CHECK(!f.Withdraw(*p1, "nope", 100).has_value());
    CHECK(f.Harvest(*p1, "alice").has_value());
    CHECK(!f.GetPool(99999).has_value());
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
'''
}

# ==================== voting_escrow ====================
MODULES["voting_escrow"] = {
"h": r'''#ifndef SHIVRAI_HON_VOTING_ESCROW_H
#define SHIVRAI_HON_VOTING_ESCROW_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
namespace shivrai::voting_escrow {
using LockId = uint64_t;
static constexpr uint32_t BPS = 10000;
static constexpr int64_t MAX_LOCK_SEC = 4 * 365 * 24 * 3600;
struct LockEntry {
    LockId id{0};
    std::string owner;
    uint64_t amount{0};
    int64_t unlock_at{0};
    int64_t created_at{0};
};
class VotingEscrow {
public:
    VotingEscrow();
    std::optional<LockId> Lock(const std::string& owner, uint64_t amount, int64_t duration_sec);
    bool IncreaseAmount(LockId id, const std::string& owner, uint64_t amount);
    bool ExtendLock(LockId id, const std::string& owner, int64_t new_duration_sec);
    bool Withdraw(LockId id, const std::string& owner);
    uint64_t VotingPower(LockId id) const;
    uint64_t TotalVotingPowerOf(const std::string& owner) const;
    uint64_t TotalLocked() const;
    uint64_t TotalVotingPower() const;
    std::optional<LockEntry> Get(LockId id) const;
    std::vector<LockId> LocksOf(const std::string& owner) const;
    size_t LockCount() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<LockId, LockEntry> locks_;
    std::map<std::string, std::vector<LockId>> by_owner_;
    LockId next_id_{1};
};
}
#endif
''',
"cpp": r'''#include "voting_escrow.h"
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
''',
"test": r'''#include "shivrai/voting_escrow/voting_escrow.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::voting_escrow;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    VotingEscrow ve;
    CHECK(ve.LockCount() == 0);
    CHECK(ve.TotalLocked() == 0);
    CHECK(!ve.Lock("", 100, 100).has_value());
    CHECK(!ve.Lock("alice", 0, 100).has_value());
    CHECK(!ve.Lock("alice", 100, 0).has_value());
    CHECK(!ve.Lock("alice", 100, MAX_LOCK_SEC + 1).has_value());
    auto l1 = ve.Lock("alice", 1000, MAX_LOCK_SEC);
    CHECK(l1.has_value());
    CHECK(ve.LockCount() == 1);
    CHECK(ve.Get(*l1)->owner == "alice");
    CHECK(ve.Get(*l1)->amount == 1000);
    CHECK(ve.LocksOf("alice").size() == 1);
    CHECK(ve.TotalLocked() == 1000);
    // At max lock, voting power ≈ amount
    uint64_t vp = ve.VotingPower(*l1);
    CHECK(vp > 0);
    CHECK(ve.TotalVotingPowerOf("alice") >= vp - 1);
    CHECK(ve.TotalVotingPower() >= vp - 1);
    CHECK(ve.IncreaseAmount(*l1, "alice", 500));
    CHECK(ve.Get(*l1)->amount == 1500);
    CHECK(!ve.IncreaseAmount(*l1, "bob", 100));
    CHECK(!ve.IncreaseAmount(*l1, "alice", 0));
    CHECK(ve.ExtendLock(*l1, "alice", MAX_LOCK_SEC / 2));
    CHECK(!ve.ExtendLock(*l1, "bob", 100));
    CHECK(!ve.ExtendLock(*l1, "alice", 0));
    CHECK(!ve.Withdraw(*l1, "alice"));  // still locked
    CHECK(!ve.Withdraw(*l1, "bob"));
    CHECK(!ve.Get(99999).has_value());
    auto l2 = ve.Lock("bob", 2000, MAX_LOCK_SEC);
    CHECK(l2.has_value());
    CHECK(ve.TotalLocked() == 3500);
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
'''
}

# ==================== atomic_swap ====================
MODULES["atomic_swap"] = {
"h": r'''#ifndef SHIVRAI_HON_ATOMIC_SWAP_H
#define SHIVRAI_HON_ATOMIC_SWAP_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
namespace shivrai::atomic_swap {
using SwapId = uint64_t;
enum class SwapState { Open, Claimed, Refunded };
struct HtlcSwap {
    SwapId id{0};
    std::string initiator, participant;
    std::string from_chain, to_chain;
    std::string asset;
    uint64_t amount{0};
    std::string hash_lock;      // H = hash(secret)
    std::string secret;         // revealed on claim
    int64_t timeout{0};
    int64_t created_at{0};
    SwapState state{SwapState::Open};
};
class AtomicSwap {
public:
    AtomicSwap();
    std::optional<SwapId> Initiate(const std::string& initiator,
                                   const std::string& participant,
                                   const std::string& from_chain,
                                   const std::string& to_chain,
                                   const std::string& asset,
                                   uint64_t amount,
                                   const std::string& hash_lock,
                                   int64_t timeout_sec);
    bool Claim(SwapId id, const std::string& caller, const std::string& secret);
    bool Refund(SwapId id, const std::string& caller);
    std::optional<HtlcSwap> Get(SwapId id) const;
    size_t Count() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<SwapId, HtlcSwap> swaps_;
    SwapId next_id_{1};
};
}
#endif
''',
"cpp": r'''#include "atomic_swap.h"
#include <ctime>
namespace shivrai::atomic_swap {
AtomicSwap::AtomicSwap() = default;
std::optional<SwapId> AtomicSwap::Initiate(const std::string& ini, const std::string& p,
                                           const std::string& fc, const std::string& tc,
                                           const std::string& a, uint64_t amt,
                                           const std::string& hl, int64_t tos) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (ini.empty() || p.empty() || ini == p) return std::nullopt;
    if (fc.empty() || tc.empty() || fc == tc) return std::nullopt;
    if (a.empty() || amt == 0 || hl.empty() || tos <= 0) return std::nullopt;
    SwapId id = next_id_++;
    HtlcSwap s;
    s.id = id; s.initiator = ini; s.participant = p;
    s.from_chain = fc; s.to_chain = tc; s.asset = a; s.amount = amt;
    s.hash_lock = hl;
    s.created_at = (int64_t)std::time(nullptr);
    s.timeout = s.created_at + tos;
    swaps_[id] = s;
    return id;
}
bool AtomicSwap::Claim(SwapId id, const std::string& caller, const std::string& secret) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = swaps_.find(id);
    if (it == swaps_.end()) return false;
    if (it->second.state != SwapState::Open) return false;
    if (it->second.participant != caller) return false;
    if (secret.empty()) return false;
    int64_t now = (int64_t)std::time(nullptr);
    if (now >= it->second.timeout) return false;
    it->second.secret = secret;
    it->second.state = SwapState::Claimed;
    return true;
}
bool AtomicSwap::Refund(SwapId id, const std::string& caller) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = swaps_.find(id);
    if (it == swaps_.end()) return false;
    if (it->second.state != SwapState::Open) return false;
    if (it->second.initiator != caller) return false;
    int64_t now = (int64_t)std::time(nullptr);
    if (now < it->second.timeout) return false;
    it->second.state = SwapState::Refunded;
    return true;
}
std::optional<HtlcSwap> AtomicSwap::Get(SwapId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = swaps_.find(id);
    if (it == swaps_.end()) return std::nullopt;
    return it->second;
}
size_t AtomicSwap::Count() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return swaps_.size();
}
}
''',
"test": r'''#include "shivrai/atomic_swap/atomic_swap.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::atomic_swap;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    AtomicSwap as;
    CHECK(as.Count() == 0);
    CHECK(!as.Initiate("", "b", "eth", "shv", "USDC", 100, "0xhash", 100).has_value());
    CHECK(!as.Initiate("a", "b", "eth", "eth", "USDC", 100, "0xhash", 100).has_value());
    CHECK(!as.Initiate("a", "b", "eth", "shv", "", 100, "0xhash", 100).has_value());
    CHECK(!as.Initiate("a", "b", "eth", "shv", "USDC", 0, "0xhash", 100).has_value());
    CHECK(!as.Initiate("a", "b", "eth", "shv", "USDC", 100, "", 100).has_value());
    CHECK(!as.Initiate("a", "b", "eth", "shv", "USDC", 100, "0xhash", 0).has_value());
    auto s = as.Initiate("alice", "bob", "eth", "shv", "USDC", 1000, "0xabc", 3600);
    CHECK(s.has_value());
    CHECK(as.Count() == 1);
    auto sw = as.Get(*s);
    CHECK(sw.has_value());
    CHECK(sw->initiator == "alice");
    CHECK(sw->participant == "bob");
    CHECK(sw->amount == 1000);
    CHECK(sw->hash_lock == "0xabc");
    CHECK(sw->state == SwapState::Open);
    CHECK(!as.Claim(*s, "alice", "secret"));
    CHECK(!as.Claim(*s, "bob", ""));
    CHECK(as.Claim(*s, "bob", "secret"));
    CHECK(as.Get(*s)->state == SwapState::Claimed);
    CHECK(!as.Claim(*s, "bob", "again"));
    CHECK(!as.Refund(*s, "alice"));
    CHECK(!as.Get(99999).has_value());
    auto s2 = as.Initiate("c", "d", "eth", "shv", "USDC", 500, "0xdef", 1);
    CHECK(s2.has_value());
    CHECK(!as.Refund(*s2, "x"));
    CHECK(!as.Refund(*s2, "c"));
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
'''
}

# ==================== lottery ====================
MODULES["lottery"] = {
"h": r'''#ifndef SHIVRAI_HON_LOTTERY_H
#define SHIVRAI_HON_LOTTERY_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>
namespace shivrai::lottery {
using RoundId = uint64_t;
struct Ticket { std::string buyer; uint64_t count{0}; };
struct Round {
    RoundId id{0};
    uint64_t ticket_price{0};
    uint64_t total_tickets{0};
    uint64_t jackpot{0};
    int64_t opened_at{0};
    int64_t closed_at{0};
    std::string winner;
    bool drawn{false};
};
class Lottery {
public:
    Lottery();
    std::optional<RoundId> OpenRound(uint64_t ticket_price);
    bool BuyTickets(RoundId id, const std::string& buyer, uint64_t count);
    bool CloseRound(RoundId id);
    bool DrawWinner(RoundId id, uint64_t seed);
    std::optional<Round> GetRound(RoundId id) const;
    uint64_t TicketsOf(RoundId id, const std::string& buyer) const;
    size_t RoundCount() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<RoundId, Round> rounds_;
    std::map<RoundId, std::map<std::string, uint64_t>> tickets_;
    std::map<RoundId, std::vector<std::string>> ticket_order_;
    RoundId next_id_{1};
};
}
#endif
''',
"cpp": r'''#include "lottery.h"
#include <ctime>
namespace shivrai::lottery {
Lottery::Lottery() = default;
std::optional<RoundId> Lottery::OpenRound(uint64_t tp) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (tp == 0) return std::nullopt;
    RoundId id = next_id_++;
    Round r;
    r.id = id; r.ticket_price = tp;
    r.opened_at = (int64_t)std::time(nullptr);
    rounds_[id] = r;
    return id;
}
bool Lottery::BuyTickets(RoundId id, const std::string& buyer, uint64_t count) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = rounds_.find(id);
    if (it == rounds_.end() || buyer.empty() || count == 0) return false;
    if (it->second.closed_at != 0) return false;
    it->second.total_tickets += count;
    it->second.jackpot += it->second.ticket_price * count;
    tickets_[id][buyer] += count;
    for (uint64_t i = 0; i < count; ++i) ticket_order_[id].push_back(buyer);
    return true;
}
bool Lottery::CloseRound(RoundId id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = rounds_.find(id);
    if (it == rounds_.end() || it->second.closed_at != 0) return false;
    it->second.closed_at = (int64_t)std::time(nullptr);
    return true;
}
bool Lottery::DrawWinner(RoundId id, uint64_t seed) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = rounds_.find(id);
    if (it == rounds_.end() || it->second.drawn) return false;
    if (it->second.total_tickets == 0) return false;
    auto& order = ticket_order_[id];
    uint64_t idx = seed % order.size();
    it->second.winner = order[idx];
    it->second.drawn = true;
    return true;
}
std::optional<Round> Lottery::GetRound(RoundId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = rounds_.find(id);
    if (it == rounds_.end()) return std::nullopt;
    return it->second;
}
uint64_t Lottery::TicketsOf(RoundId id, const std::string& buyer) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = tickets_.find(id);
    if (it == tickets_.end()) return 0;
    auto uit = it->second.find(buyer);
    return uit == it->second.end() ? 0 : uit->second;
}
size_t Lottery::RoundCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return rounds_.size();
}
}
''',
"test": r'''#include "shivrai/lottery/lottery.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::lottery;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    Lottery l;
    CHECK(l.RoundCount() == 0);
    CHECK(!l.OpenRound(0).has_value());
    auto r1 = l.OpenRound(10);
    CHECK(r1.has_value());
    CHECK(l.RoundCount() == 1);
    CHECK(l.GetRound(*r1)->ticket_price == 10);
    CHECK(l.GetRound(*r1)->total_tickets == 0);
    CHECK(!l.BuyTickets(*r1, "", 1));
    CHECK(!l.BuyTickets(*r1, "alice", 0));
    CHECK(!l.BuyTickets(99999, "alice", 1));
    CHECK(l.BuyTickets(*r1, "alice", 5));
    CHECK(l.BuyTickets(*r1, "bob", 3));
    CHECK(l.TicketsOf(*r1, "alice") == 5);
    CHECK(l.TicketsOf(*r1, "bob") == 3);
    CHECK(l.TicketsOf(*r1, "carol") == 0);
    CHECK(l.GetRound(*r1)->total_tickets == 8);
    CHECK(l.GetRound(*r1)->jackpot == 80);
    CHECK(l.CloseRound(*r1));
    CHECK(!l.CloseRound(*r1));
    CHECK(!l.BuyTickets(*r1, "carol", 1));
    CHECK(l.DrawWinner(*r1, 0));
    CHECK(l.GetRound(*r1)->drawn);
    CHECK(!l.GetRound(*r1)->winner.empty());
    CHECK(!l.DrawWinner(*r1, 1));
    auto r2 = l.OpenRound(100);
    CHECK(r2.has_value());
    CHECK(!l.DrawWinner(*r2, 0));   // no tickets
    CHECK(!l.GetRound(99999).has_value());
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
'''
}

# ==================== fee_market ====================
MODULES["fee_market"] = {
"h": r'''#ifndef SHIVRAI_HON_FEE_MARKET_H
#define SHIVRAI_HON_FEE_MARKET_H
#include <cstdint>
#include <deque>
#include <map>
#include <mutex>
#include <optional>
#include <string>
namespace shivrai::fee_market {
static constexpr uint64_t GWEI = 1'000'000'000ULL;
struct Tx {
    std::string hash;
    uint64_t max_fee{0};
    uint64_t priority_fee{0};
    uint64_t gas_used{0};
    uint64_t base_fee{0};
};
class FeeMarket {
public:
    FeeMarket();
    void SetTargetBlockGas(uint64_t g);
    void SetMaxBlockGas(uint64_t g);
    uint64_t CurrentBaseFee() const;
    uint64_t BaseFeeFor(uint64_t parent_gas_used, uint64_t parent_base_fee) const;
    bool SubmitTx(const std::string& hash, uint64_t max_fee, uint64_t priority_fee, uint64_t gas_limit);
    std::optional<Tx> GetTx(const std::string& hash) const;
    size_t PendingCount() const;
    uint64_t EffectiveTip(uint64_t max_fee, uint64_t priority_fee, uint64_t base_fee) const;
    bool MineBlock();
    size_t BlocksMined() const;
    uint64_t LastBlockGasUsed() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<std::string, Tx> txs_;
    std::deque<std::string> queue_;
    uint64_t target_gas_{15'000'000};
    uint64_t max_gas_{30'000'000};
    uint64_t base_fee_{GWEI};
    size_t blocks_mined_{0};
    uint64_t last_block_gas_{0};
};
}
#endif
''',
"cpp": r'''#include "fee_market.h"
#include <vector>
namespace shivrai::fee_market {
FeeMarket::FeeMarket() = default;
void FeeMarket::SetTargetBlockGas(uint64_t g) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (g > 0) target_gas_ = g;
}
void FeeMarket::SetMaxBlockGas(uint64_t g) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (g > 0) max_gas_ = g;
}
uint64_t FeeMarket::CurrentBaseFee() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return base_fee_;
}
uint64_t FeeMarket::BaseFeeFor(uint64_t parent_gas, uint64_t parent_base) const {
    if (parent_gas == target_gas_) return parent_base;
    if (parent_gas > target_gas_) {
        uint64_t delta = parent_gas - target_gas_;
        uint64_t bump = (uint64_t)((__uint128_t)parent_base * delta / target_gas_ / 8);
        return parent_base + bump;
    }
    uint64_t delta = target_gas_ - parent_gas;
    uint64_t drop = (uint64_t)((__uint128_t)parent_base * delta / target_gas_ / 8);
    return parent_base > drop ? parent_base - drop : 0;
}
uint64_t FeeMarket::EffectiveTip(uint64_t max_fee, uint64_t priority_fee, uint64_t base_fee) const {
    if (max_fee < base_fee) return 0;
    uint64_t available = max_fee - base_fee;
    return available < priority_fee ? available : priority_fee;
}
bool FeeMarket::SubmitTx(const std::string& h, uint64_t mf, uint64_t pf, uint64_t gl) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (h.empty() || gl == 0 || gl > max_gas_) return false;
    if (txs_.count(h)) return false;
    Tx t; t.hash = h; t.max_fee = mf; t.priority_fee = pf;
    t.gas_used = gl; t.base_fee = base_fee_;
    txs_[h] = t;
    queue_.push_back(h);
    return true;
}
std::optional<Tx> FeeMarket::GetTx(const std::string& h) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = txs_.find(h);
    if (it == txs_.end()) return std::nullopt;
    return it->second;
}
size_t FeeMarket::PendingCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return queue_.size();
}
bool FeeMarket::MineBlock() {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    uint64_t used = 0;
    std::vector<std::string> mined;
    while (!queue_.empty() && used < max_gas_) {
        auto h = queue_.front();
        auto it = txs_.find(h);
        if (it == txs_.end()) { queue_.pop_front(); continue; }
        if (it->second.max_fee < base_fee_) { queue_.pop_front(); continue; }
        if (used + it->second.gas_used > max_gas_) break;
        used += it->second.gas_used;
        mined.push_back(h);
        queue_.pop_front();
    }
    last_block_gas_ = used;
    base_fee_ = BaseFeeFor(used, base_fee_);
    blocks_mined_++;
    return true;
}
size_t FeeMarket::BlocksMined() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return blocks_mined_;
}
uint64_t FeeMarket::LastBlockGasUsed() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return last_block_gas_;
}
}
''',
"test": r'''#include "shivrai/fee_market/fee_market.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::fee_market;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    FeeMarket fm;
    CHECK(fm.CurrentBaseFee() == GWEI);
    CHECK(fm.BaseFeeFor(15'000'000, 1000) == 1000); // target
    CHECK(fm.BaseFeeFor(30'000'000, 1000) == 1125); // double → +12.5%
    CHECK(fm.BaseFeeFor(0, 1000) == 875);           // empty → -12.5%
    CHECK(fm.PendingCount() == 0);
    CHECK(fm.BlocksMined() == 0);
    CHECK(!fm.SubmitTx("", 100, 1, 21000));
    CHECK(!fm.SubmitTx("0x1", 100, 1, 0));
    CHECK(fm.SubmitTx("0x1", 200 * GWEI, 2 * GWEI, 21000));
    CHECK(fm.SubmitTx("0x2", 200 * GWEI, 3 * GWEI, 21000));
    CHECK(!fm.SubmitTx("0x1", 200 * GWEI, 2 * GWEI, 21000)); // dup
    CHECK(fm.PendingCount() == 2);
    CHECK(fm.GetTx("0x1").has_value());
    CHECK(fm.GetTx("0x1")->max_fee == 200 * GWEI);
    CHECK(!fm.GetTx("nope").has_value());
    CHECK(fm.EffectiveTip(200 * GWEI, 2 * GWEI, GWEI) == 2 * GWEI);
    CHECK(fm.EffectiveTip(GWEI, 2 * GWEI, 200 * GWEI) == 0);  // max_fee < base
    CHECK(fm.EffectiveTip(2 * GWEI, 5 * GWEI, GWEI) == GWEI); // capped by available
    CHECK(fm.MineBlock());
    CHECK(fm.BlocksMined() == 1);
    CHECK(fm.PendingCount() == 0);
    CHECK(fm.LastBlockGasUsed() == 42000);
    CHECK(fm.CurrentBaseFee() < GWEI);  // < target → dropped
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
'''
}

# ==================== RUN ====================
def write_module(name, files):
    write(f"{ROOT}/src/shivrai/{name}/{name}.h", files["h"])
    write(f"{ROOT}/src/shivrai/{name}/{name}.cpp", files["cpp"])
    write(f"{ROOT}/tests/shivrai/test_{name}.cpp", files["test"])

def test_module(name):
    c = f"{ROOT}/src/shivrai/{name}/{name}.cpp"
    t = f"{ROOT}/tests/shivrai/test_{name}.cpp"
    r = subprocess.run(["g++","-std=c++17","-Wall","-Wextra","-O2",
                        f"-I{ROOT}/src", t, c, "-o", f"/tmp/t_{name}"],
                       capture_output=True, text=True)
    if r.returncode != 0:
        print(f"❌ COMPILE FAIL {name}:\n{r.stderr[:1500]}")
        return False
    r = subprocess.run([f"/tmp/t_{name}"], capture_output=True, text=True)
    if r.returncode != 0:
        print(f"❌ TEST FAIL {name}:\n{r.stdout}\n{r.stderr}")
        return False
    print(f"✅ {name}: {r.stdout.strip()}")
    return True

def main():
    target = sys.argv[1] if len(sys.argv) > 1 else None
    names = [target] if target else list(MODULES.keys())
    ok, fail = [], []
    for n in names:
        if n not in MODULES:
            print(f"Unknown module: {n}"); continue
        print(f"\n=== {n} ===")
        write_module(n, MODULES[n])
        (ok if test_module(n) else fail).append(n)
    print(f"\n=== SUMMARY ===\nOK: {ok}\nFAIL: {fail}")
    if fail: sys.exit(1)
    subprocess.run(["git","-C",ROOT,"add","-A"])
    subprocess.run(["git","-C",ROOT,"commit","-m",f"feat: batch2 ({', '.join(ok)})"])
    subprocess.run(["git","-C",ROOT,"push","origin","master"])
    print("🚀 pushed")

if __name__ == "__main__":
    main()