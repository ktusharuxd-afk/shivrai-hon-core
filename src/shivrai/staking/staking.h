#ifndef SHIVRAI_HON_STAKING_H
#define SHIVRAI_HON_STAKING_H

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace shivrai::staking {

using ValidatorId = std::string;
using DelegationId = uint64_t;

static constexpr uint32_t BPS = 10000;
static constexpr uint64_t MIN_SELF_STAKE = 10000;
static constexpr uint64_t MIN_DELEGATION = 100;
static constexpr uint32_t DEFAULT_COMMISSION_BPS = 500;
static constexpr int64_t  UNBONDING_PERIOD_SEC = 21 * 24 * 3600;
static constexpr uint32_t SLASH_DOUBLE_SIGN_BPS = 500;
static constexpr uint32_t SLASH_DOWNTIME_BPS    = 10;

enum class ValidatorStatus { Active, Jailed, Unbonding, Tombstoned };

struct Validator {
    ValidatorId id;
    std::string moniker;
    uint64_t    self_stake{0};
    uint64_t    delegated{0};
    uint64_t    total_stake{0};
    uint32_t    commission_bps{DEFAULT_COMMISSION_BPS};
    ValidatorStatus status{ValidatorStatus::Active};
    int64_t     created_at{0};
    int64_t     unbond_start{0};
    int64_t     jailed_until{0};
    uint64_t    rewards_accumulated{0};
};

struct Delegation {
    DelegationId id{0};
    std::string  delegator;
    ValidatorId  validator;
    uint64_t     amount{0};
    int64_t      created_at{0};
    int64_t      unbond_start{0};
    bool         unbonding{false};
};

class StakingPool {
public:
    StakingPool();

    bool RegisterValidator(const std::string& moniker,
                           const std::string& validator_id,
                           uint64_t self_stake,
                           uint32_t commission_bps);
    bool UpdateCommission(const std::string& validator_id, uint32_t commission_bps);
    bool JailValidator(const std::string& validator_id, int64_t duration_sec);
    bool UnjailValidator(const std::string& validator_id);
    std::optional<Validator> GetValidator(const std::string& validator_id) const;
    std::vector<std::string> AllValidators() const;
    size_t ValidatorCount() const;

    std::optional<DelegationId> Delegate(const std::string& delegator,
                                         const std::string& validator_id,
                                         uint64_t amount);
    bool BeginUndelegate(const std::string& delegator, DelegationId id);
    bool CompleteUndelegate(DelegationId id);
    std::optional<Delegation> GetDelegation(DelegationId id) const;
    std::vector<DelegationId> DelegationsOf(const std::string& delegator) const;
    std::vector<DelegationId> DelegationsTo(const std::string& validator_id) const;
    uint64_t DelegatedBy(const std::string& delegator) const;

    void DistributeRewards(const std::string& validator_id, uint64_t amount);
    uint64_t ClaimRewards(const std::string& delegator, DelegationId id);
    uint64_t PendingRewards(const std::string& delegator, DelegationId id) const;

    bool SlashDoubleSign(const std::string& validator_id);
    bool SlashDowntime(const std::string& validator_id);
    uint64_t TotalSlashed() const;

    uint64_t TotalStaked() const;
    uint64_t TotalBonded() const;
    size_t   DelegationCount() const;
    uint32_t ValidatorCountActive() const;

private:
    mutable std::recursive_mutex cs_;
    std::map<std::string, Validator>           validators_;
    std::map<DelegationId, Delegation>         delegations_;
    std::map<std::string, std::vector<DelegationId>> by_delegator_;
    std::map<std::string, std::vector<DelegationId>> by_validator_;
    std::map<DelegationId, uint64_t>           pending_rewards_;

    DelegationId next_delegation_id_{1};
    uint64_t     total_slashed_{0};
};

}
#endif