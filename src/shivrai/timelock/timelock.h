#ifndef SHIVRAI_HON_TIMELOCK_H
#define SHIVRAI_HON_TIMELOCK_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
namespace shivrai::timelock {
using OpId = uint64_t;
enum class OpState { Queued, Executed, Cancelled };
struct Operation {
    OpId id{0}; std::string proposer, target, payload;
    int64_t queued_at{0}, eta{0}, executed_at{0};
    OpState state{OpState::Queued};
};
class Timelock {
public:
    Timelock();
    void SetDelaySec(int64_t s);
    int64_t DelaySec() const;
    std::optional<OpId> Queue(const std::string&, const std::string&, const std::string&);
    bool Execute(OpId);
    bool Cancel(OpId, const std::string&);
    std::optional<Operation> Get(OpId) const;
    size_t Count() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<OpId, Operation> ops_;
    OpId next_id_{1};
    int64_t delay_sec_{3600};
};
}
#endif
