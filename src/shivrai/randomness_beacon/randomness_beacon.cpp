#include "randomness_beacon.h"
#include <ctime>
namespace shivrai::randomness_beacon {
Beacon::Beacon() = default;
std::optional<RoundId> Beacon::OpenRound(const std::string& seed) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (seed.empty()) return std::nullopt;
    RoundId id = next_id_++;
    Round r;
    r.id = id; r.seed = seed;
    r.opened_at = (int64_t)std::time(nullptr);
    rounds_[id] = r;
    return id;
}
bool Beacon::Commit(RoundId id, const std::string& p, const std::string& c) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = rounds_.find(id);
    if (it == rounds_.end() || it->second.finalized) return false;
    if (p.empty() || c.empty()) return false;
    if (it->second.commitments.count(p)) return false;
    it->second.commitments.insert(p);
    return true;
}
bool Beacon::Reveal(RoundId id, const std::string& p, const std::string& v) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = rounds_.find(id);
    if (it == rounds_.end() || it->second.finalized) return false;
    if (!it->second.commitments.count(p)) return false;
    if (it->second.reveals.count(p)) return false;
    it->second.reveals[p] = v;
    return true;
}
bool Beacon::Finalize(RoundId id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = rounds_.find(id);
    if (it == rounds_.end() || it->second.finalized) return false;
    if (it->second.reveals.empty()) return false;
    it->second.result = Combine(id);
    it->second.revealed_at = (int64_t)std::time(nullptr);
    it->second.finalized = true;
    return true;
}
std::optional<Round> Beacon::GetRound(RoundId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = rounds_.find(id);
    if (it == rounds_.end()) return std::nullopt;
    return it->second;
}
std::string Beacon::Combine(RoundId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = rounds_.find(id);
    if (it == rounds_.end()) return "";
    std::string combined = it->second.seed;
    for (auto& [p, v] : it->second.reveals) combined += "|" + p + ":" + v;
    uint64_t h = 1469598103934665603ULL;
    for (char c : combined) { h ^= (uint8_t)c; h *= 1099511628211ULL; }
    char buf[32];
    snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)h);
    return std::string(buf);
}
size_t Beacon::RoundCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return rounds_.size();
}
}
