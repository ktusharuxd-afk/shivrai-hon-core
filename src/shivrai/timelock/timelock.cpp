#include "timelock.h"
#include <ctime>
namespace shivrai::timelock {
Timelock::Timelock() = default;
void Timelock::SetDelaySec(int64_t s) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (s >= 0) delay_sec_ = s;
}
int64_t Timelock::DelaySec() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return delay_sec_;
}
std::optional<OpId> Timelock::Queue(const std::string& p, const std::string& t, const std::string& pl) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (p.empty() || t.empty()) return std::nullopt;
    OpId id = next_id_++;
    Operation op;
    op.id = id; op.proposer = p; op.target = t; op.payload = pl;
    op.queued_at = (int64_t)std::time(nullptr);
    op.eta = op.queued_at + delay_sec_;
    ops_[id] = op;
    return id;
}
bool Timelock::Execute(OpId id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = ops_.find(id);
    if (it == ops_.end()) return false;
    if (it->second.state != OpState::Queued) return false;
    int64_t now = (int64_t)std::time(nullptr);
    if (now < it->second.eta) return false;
    it->second.executed_at = now;
    it->second.state = OpState::Executed;
    return true;
}
bool Timelock::Cancel(OpId id, const std::string& c) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = ops_.find(id);
    if (it == ops_.end()) return false;
    if (it->second.state != OpState::Queued) return false;
    if (it->second.proposer != c) return false;
    it->second.state = OpState::Cancelled;
    return true;
}
std::optional<Operation> Timelock::Get(OpId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = ops_.find(id);
    if (it == ops_.end()) return std::nullopt;
    return it->second;
}
size_t Timelock::Count() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return ops_.size();
}
}
