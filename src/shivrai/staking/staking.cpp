#include "staking.h"
#include <algorithm>
#include <ctime>

namespace shivrai::staking {

StakingPool::StakingPool() = default;

bool StakingPool::RegisterValidator(const std::string& moniker,
                                    const std::string& validator_id,
                                    uint64_t self_stake,
                                    uint32_t commission_bps) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (moniker.empty() || validator_id.empty()) return false;
    if (validators_.count(validator_id)) return false;
    if (self_stake < MIN_SELF_STAKE) return false;
    if (commission_bps > BPS) return false;

    Validator v;
    v.id = validator_id;
    v.moniker = moniker;
    v.self_stake = self_stake;
    v.total_stake = self_stake;
    v.commission_bps = commission_bps;
    v.status = ValidatorStatus::Active;
    v.created_at = (int64_t)std::time(nullptr);
    validators_[validator_id] = v;
    return true;
}

bool StakingPool::UpdateCommission(const std::string& validator_id, uint32_t commission_bps) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (commission_bps > BPS) return false;
    auto it = validators_.find(validator_id);
    if (it == validators_.end()) return false;
    if (it->second.status != ValidatorStatus::Active) return false;
    it->second.commission_bps = commission_bps;
    return true;
}

bool StakingPool::JailValidator(const std::string& validator_id, int64_t duration_sec) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = validators_.find(validator_id);
    if (it == validators_.end()) return false;
    if (it->second.status == ValidatorStatus::Tombstoned) return false;
    it->second.status = ValidatorStatus::Jailed;
    it->second.jailed_until = (int64_t)std::time(nullptr) + duration_sec;
    return true;
}

bool StakingPool::UnjailValidator(const std::string& validator_id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = validators_.find(validator_id);
    if (it == validators_.end()) return false;
    if (it->second.status != ValidatorStatus::Jailed) return false;
    it->second.status = ValidatorStatus::Active;
    it->second.jailed_until = 0;
    return true;
}

std::optional<Validator> StakingPool::GetValidator(const std::string& validator_id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = validators_.find(validator_id);
    if (it == validators_.end()) return std::nullopt;
    return it->second;
}

std::vector<std::string> StakingPool::AllValidators() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<std::string> out;
    for (auto& [k, v] : validators_) out.push_back(k);
    return out;
}

size_t StakingPool::ValidatorCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return validators_.size();
}

uint32_t StakingPool::ValidatorCountActive() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    uint32_t n = 0;
    for (auto& [k, v] : validators_) if (v.status == ValidatorStatus::Active) ++n;
    return n;
}

std::optional<DelegationId> StakingPool::Delegate(const std::string& delegator,
                                                  const std::string& validator_id,
                                                  uint64_t amount) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (delegator.empty() || amount < MIN_DELEGATION) return std::nullopt;
    auto vit = validators_.find(validator_id);
    if (vit == validators_.end()) return std::nullopt;
    if (vit->second.status != ValidatorStatus::Active) return std::nullopt;

    DelegationId id = next_delegation_id_++;
    Delegation d;
    d.id = id;
    d.delegator = delegator;
    d.validator = validator_id;
    d.amount = amount;
    d.created_at = (int64_t)std::time(nullptr);

    delegations_[id] = d;
    by_delegator_[delegator].push_back(id);
    by_validator_[validator_id].push_back(id);
    vit->second.delegated += amount;
    vit->second.total_stake += amount;
    return id;
}

bool StakingPool::BeginUndelegate(const std::string& delegator, DelegationId id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = delegations_.find(id);
    if (it == delegations_.end()) return false;
    if (it->second.delegator != delegator) return false;
    if (it->second.unbonding) return false;
    it->second.unbonding = true;
    it->second.unbond_start = (int64_t)std::time(nullptr);
    return true;
}

bool StakingPool::CompleteUndelegate(DelegationId id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = delegations_.find(id);
    if (it == delegations_.end()) return false;
    if (!it->second.unbonding) return false;
    int64_t now = (int64_t)std::time(nullptr);
    if (now - it->second.unbond_start < UNBONDING_PERIOD_SEC) return false;

    auto vit = validators_.find(it->second.validator);
    if (vit != validators_.end()) {
        if (vit->second.delegated >= it->second.amount) vit->second.delegated -= it->second.amount;
        if (vit->second.total_stake >= it->second.amount) vit->second.total_stake -= it->second.amount;
    }
    delegations_.erase(it);
    pending_rewards_.erase(id);
    return true;
}

std::optional<Delegation> StakingPool::GetDelegation(DelegationId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = delegations_.find(id);
    if (it == delegations_.end()) return std::nullopt;
    return it->second;
}

std::vector<DelegationId> StakingPool::DelegationsOf(const std::string& delegator) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = by_delegator_.find(delegator);
    return it == by_delegator_.end() ? std::vector<DelegationId>{} : it->second;
}

std::vector<DelegationId> StakingPool::DelegationsTo(const std::string& validator_id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = by_validator_.find(validator_id);
    return it == by_validator_.end() ? std::vector<DelegationId>{} : it->second;
}

uint64_t StakingPool::DelegatedBy(const std::string& delegator) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    uint64_t total = 0;
    auto it = by_delegator_.find(delegator);
    if (it != by_delegator_.end()) {
        for (auto id : it->second) {
            auto dit = delegations_.find(id);
            if (dit != delegations_.end()) total += dit->second.amount;
        }
    }
    return total;
}

void StakingPool::DistributeRewards(const std::string& validator_id, uint64_t amount) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto vit = validators_.find(validator_id);
    if (vit == validators_.end()) return;
    if (vit->second.status == ValidatorStatus::Tombstoned) return;

    uint64_t commission = (uint64_t)((__uint128_t)amount * vit->second.commission_bps / BPS);
    uint64_t to_delegators = amount - commission;
    vit->second.rewards_accumulated += commission;

    auto dit = by_validator_.find(validator_id);
    if (dit == by_validator_.end() || dit->second.empty()) return;

    uint64_t total_delegated = vit->second.delegated;
    if (total_delegated == 0) return;

    uint64_t distributed = 0;
    auto& ids = dit->second;
    for (size_t i = 0; i < ids.size(); ++i) {
        auto d = delegations_.find(ids[i]);
        if (d == delegations_.end()) continue;
        uint64_t share;
        if (i == ids.size() - 1) {
            share = to_delegators - distributed;
        } else {
            share = (uint64_t)((__uint128_t)to_delegators * d->second.amount / total_delegated);
            distributed += share;
        }
        pending_rewards_[ids[i]] += share;
    }
}

uint64_t StakingPool::ClaimRewards(const std::string& delegator, DelegationId id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = delegations_.find(id);
    if (it == delegations_.end()) return 0;
    if (it->second.delegator != delegator) return 0;
    auto rit = pending_rewards_.find(id);
    if (rit == pending_rewards_.end()) return 0;
    uint64_t r = rit->second;
    rit->second = 0;
    return r;
}

uint64_t StakingPool::PendingRewards(const std::string& delegator, DelegationId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = delegations_.find(id);
    if (it == delegations_.end()) return 0;
    if (it->second.delegator != delegator) return 0;
    auto rit = pending_rewards_.find(id);
    return rit == pending_rewards_.end() ? 0 : rit->second;
}

bool StakingPool::SlashDoubleSign(const std::string& validator_id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto vit = validators_.find(validator_id);
    if (vit == validators_.end()) return false;
    if (vit->second.status == ValidatorStatus::Tombstoned) return false;

    uint64_t slash = (uint64_t)((__uint128_t)vit->second.total_stake * SLASH_DOUBLE_SIGN_BPS / BPS);
    if (vit->second.self_stake >= slash) vit->second.self_stake -= slash;
    else vit->second.self_stake = 0;
    if (vit->second.total_stake >= slash) vit->second.total_stake -= slash;
    else vit->second.total_stake = 0;
    total_slashed_ += slash;

    vit->second.status = ValidatorStatus::Tombstoned;
    return true;
}

bool StakingPool::SlashDowntime(const std::string& validator_id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto vit = validators_.find(validator_id);
    if (vit == validators_.end()) return false;
    if (vit->second.status == ValidatorStatus::Tombstoned) return false;

    uint64_t slash = (uint64_t)((__uint128_t)vit->second.total_stake * SLASH_DOWNTIME_BPS / BPS);
    if (vit->second.self_stake >= slash) vit->second.self_stake -= slash;
    else vit->second.self_stake = 0;
    if (vit->second.total_stake >= slash) vit->second.total_stake -= slash;
    else vit->second.total_stake = 0;
    total_slashed_ += slash;
    return true;
}

uint64_t StakingPool::TotalSlashed() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return total_slashed_;
}

uint64_t StakingPool::TotalStaked() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    uint64_t t = 0;
    for (auto& [k, v] : validators_) t += v.total_stake;
    return t;
}

uint64_t StakingPool::TotalBonded() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    uint64_t t = 0;
    for (auto& [k, v] : validators_) {
        if (v.status == ValidatorStatus::Active) t += v.total_stake;
    }
    return t;
}

size_t StakingPool::DelegationCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return delegations_.size();
}

}