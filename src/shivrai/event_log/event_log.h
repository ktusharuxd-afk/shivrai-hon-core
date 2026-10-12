#ifndef SHIVRAI_HON_EVENT_LOG_H
#define SHIVRAI_HON_EVENT_LOG_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
namespace shivrai::event_log {
using EventId = uint64_t;
struct Event { EventId id{0}; std::string topic, payload; int64_t ts{0}; };
class EventLog {
public:
    EventLog();
    EventId Emit(const std::string& topic, const std::string& payload);
    std::optional<Event> Get(EventId) const;
    std::vector<Event> ByTopic(const std::string& topic) const;
    std::vector<Event> Range(EventId from, EventId to) const;
    size_t Count() const;
    size_t TopicCount() const;
    EventId LastId() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<EventId, Event> events_;
    std::map<std::string, std::vector<EventId>> by_topic_;
    EventId next_id_{1};
};
}
#endif
