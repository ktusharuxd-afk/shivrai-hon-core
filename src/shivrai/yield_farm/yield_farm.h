#ifndef SHIVRAI_HON_YIELD_FARM_H
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
