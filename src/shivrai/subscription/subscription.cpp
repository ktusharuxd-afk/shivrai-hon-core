#include "subscription.h"
#include <ctime>
namespace shivrai::subscription {
Subscriptions::Subscriptions() = default;
std::optional<SubId> Subscriptions::Subscribe(const std::string& s, const std::string& p,
                                              const std::string& pl, uint64_t amt, int64_t iv) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (s.empty() || p.empty() || pl.empty() || amt == 0 || iv <= 0) return std::nullopt;
    if (s == p) return std::nullopt;
    SubId id = next_id_++;
    Sub sub;
    sub.id = id; sub.subscriber = s; sub.provider = p; sub.plan = pl;
    sub.amount = amt; sub.interval_sec = iv;
    sub.created_at = (int64_t)std::time(nullptr);
    sub.next_charge = sub.created_at + iv;
    subs_[id] = sub;
    by_subscriber_[s].push_back(id);
    return id;
}
bool Subscriptions::Cancel(SubId id, const std::string& c) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = subs_.find(id);
    if (it == subs_.end()) return false;
    if (it->second.subscriber != c) return false;
    if (!it->second.active) return false;
    it->second.active = false;
    return true;
}
bool Subscriptions::Pause(SubId id, const std::string& c) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = subs_.find(id);
    if (it == subs_.end() || !it->second.active) return false;
    if (it->second.subscriber != c) return false;
    it->second.active = false;
    return true;
}
bool Subscriptions::Resume(SubId id, const std::string& c) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = subs_.find(id);
    if (it == subs_.end() || it->second.active) return false;
    if (it->second.subscriber != c) return false;
    it->second.active = true;
    return true;
}
std::optional<SubId> Subscriptions::ChangePlan(SubId id, const std::string& c,
                                               const std::string& np, uint64_t na) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = subs_.find(id);
    if (it == subs_.end() || !it->second.active) return std::nullopt;
    if (it->second.subscriber != c) return std::nullopt;
    if (np.empty() || na == 0) return std::nullopt;
    if (it->second.plan == np) return std::nullopt;
    // deactivate old, create new
    it->second.active = false;
    return Subscribe(c, it->second.provider, np, na, it->second.interval_sec);
}
uint64_t Subscriptions::Charge(SubId id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = subs_.find(id);
    if (it == subs_.end() || !it->second.active) return 0;
    it->second.cycles_billed++;
    it->second.total_billed += it->second.amount;
    it->second.next_charge += it->second.interval_sec;
    return it->second.amount;
}
std::optional<Sub> Subscriptions::Get(SubId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = subs_.find(id);
    if (it == subs_.end()) return std::nullopt;
    return it->second;
}
std::vector<SubId> Subscriptions::SubsOf(const std::string& s) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = by_subscriber_.find(s);
    return it == by_subscriber_.end() ? std::vector<SubId>{} : it->second;
}
size_t Subscriptions::Count() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return subs_.size();
}
}
