#ifndef SHIVRAI_HON_VOTING_ESCROW_H
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
