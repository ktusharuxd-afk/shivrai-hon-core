#ifndef SHIVRAI_HON_TICKET_SYSTEM_H
#define SHIVRAI_HON_TICKET_SYSTEM_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
namespace shivrai::ticket_system {
using EventId  = uint64_t;
using TicketId = uint64_t;
struct Event {
    EventId id{0};
    std::string name;
    uint64_t capacity{0};
    uint64_t price{0};
    uint64_t sold{0};
    int64_t event_at{0};
    bool active{true};
    bool transferable{true};
};
struct Ticket {
    TicketId id{0};
    EventId event{0};
    std::string owner;
    uint64_t paid{0};
    bool checked_in{false};
    int64_t bought_at{0};
};
class TicketSystem {
public:
    TicketSystem();
    std::optional<EventId> CreateEvent(const std::string& name, uint64_t capacity,
                                       uint64_t price, int64_t event_at);
    bool SetTransferable(EventId id, bool t);
    bool SetActive(EventId id, bool a);
    std::optional<Event> GetEvent(EventId id) const;
    std::optional<TicketId> Buy(EventId id, const std::string& buyer);
    bool CheckIn(TicketId id, const std::string& caller);
    bool Transfer(TicketId id, const std::string& from, const std::string& to);
    std::optional<Ticket> GetTicket(TicketId id) const;
    uint64_t TicketsOwnedBy(const std::string& owner) const;
    size_t EventCount() const;
    size_t TicketCount() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<EventId, Event> events_;
    std::map<TicketId, Ticket> tickets_;
    std::map<std::string, uint64_t> owned_;
    EventId  next_event_id_{1};
    TicketId next_ticket_id_{1};
};
}
#endif
