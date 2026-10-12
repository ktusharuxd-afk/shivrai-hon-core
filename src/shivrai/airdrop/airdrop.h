#ifndef SHIVRAI_HON_AIRDROP_H
#define SHIVRAI_HON_AIRDROP_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <vector>
namespace shivrai::airdrop {
using RoundId = uint64_t;
struct AirdropRound {
    RoundId id{0};
    std::string name;
    uint64_t total_allocated{0};
    uint64_t total_claimed{0};
    int64_t starts_at{0};
    int64_t ends_at{0};
    bool active{true};
};
class Airdrop {
public:
    Airdrop();
    std::optional<RoundId> CreateRound(const std::string& name, int64_t start, int64_t end);
    bool SetEligibility(RoundId id, const std::string& user, uint64_t amount);
    std::optional<uint64_t> Claim(RoundId id, const std::string& user);
    std::optional<uint64_t> AmountFor(RoundId id, const std::string& user) const;
    bool HasClaimed(RoundId id, const std::string& user) const;
    std::optional<AirdropRound> GetRound(RoundId id) const;
    size_t RoundCount() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<RoundId, AirdropRound> rounds_;
    std::map<RoundId, std::map<std::string, uint64_t>> eligible_;
    std::map<RoundId, std::map<std::string, bool>> claimed_;
    RoundId next_id_{1};
};
}
#endif
