#ifndef SHIVRAI_HON_RANDOMNESS_BEACON_H
#define SHIVRAI_HON_RANDOMNESS_BEACON_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>
namespace shivrai::randomness_beacon {
using RoundId = uint64_t;
struct Round {
    RoundId id{0};
    std::string seed;
    std::string result;
    int64_t opened_at{0};
    int64_t revealed_at{0};
    std::set<std::string> commitments;
    std::map<std::string, std::string> reveals;
    bool finalized{false};
};
class Beacon {
public:
    Beacon();
    std::optional<RoundId> OpenRound(const std::string& seed);
    bool Commit(RoundId id, const std::string& participant, const std::string& commitment);
    bool Reveal(RoundId id, const std::string& participant, const std::string& value);
    bool Finalize(RoundId id);
    std::optional<Round> GetRound(RoundId id) const;
    std::string Combine(RoundId id) const;
    size_t RoundCount() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<RoundId, Round> rounds_;
    RoundId next_id_{1};
};
}
#endif
