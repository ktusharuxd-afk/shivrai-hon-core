#include "event_log.h"
#include <ctime>
namespace shivrai::event_log {
EventLog::EventLog() = default;
EventId EventLog::Emit(const std::string& t, const std::string& p) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (t.empty()) return 0;
    EventId id = next_id_++;
    Event e;
    e.id = id; e.topic = t; e.payload = p;
    e.ts = (int64_t)std::time(nullptr);
    events_[id] = e;
    by_topic_[t].push_back(id);
    return id;
}
std::optional<Event> EventLog::Get(EventId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = events_.find(id);
    if (it == events_.end()) return std::nullopt;
    return it->second;
}
std::vector<Event> EventLog::ByTopic(const std::string& t) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<Event> out;
    auto it = by_topic_.find(t);
    if (it == by_topic_.end()) return out;
    for (auto id : it->second) {
        auto e = events_.find(id);
        if (e != events_.end()) out.push_back(e->second);
    }
    return out;
}
std::vector<Event> EventLog::Range(EventId a, EventId b) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<Event> out;
    for (auto& [id, e] : events_) if (id >= a && id <= b) out.push_back(e);
    return out;
}
size_t EventLog::Count() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return events_.size();
}
size_t EventLog::TopicCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return by_topic_.size();
}
EventId EventLog::LastId() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return events_.empty() ? 0 : events_.rbegin()->first;
}
}
