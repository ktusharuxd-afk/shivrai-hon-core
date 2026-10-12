#include "ticket_system.h"
#include <ctime>
namespace shivrai::ticket_system {
TicketSystem::TicketSystem() = default;
std::optional<EventId> TicketSystem::CreateEvent(const std::string& n, uint64_t cap,
                                                 uint64_t pr, int64_t ea) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (n.empty() || cap == 0 || pr == 0) return std::nullopt;
    EventId id = next_event_id_++;
    Event e;
    e.id = id; e.name = n; e.capacity = cap; e.price = pr; e.event_at = ea;
    events_[id] = e;
    return id;
}
bool TicketSystem::SetTransferable(EventId id, bool t) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = events_.find(id);
    if (it == events_.end()) return false;
    it->second.transferable = t;
    return true;
}
bool TicketSystem::SetActive(EventId id, bool a) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = events_.find(id);
    if (it == events_.end()) return false;
    it->second.active = a;
    return true;
}
std::optional<Event> TicketSystem::GetEvent(EventId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = events_.find(id);
    if (it == events_.end()) return std::nullopt;
    return it->second;
}
std::optional<TicketId> TicketSystem::Buy(EventId id, const std::string& buyer) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = events_.find(id);
    if (it == events_.end() || !it->second.active) return std::nullopt;
    if (buyer.empty()) return std::nullopt;
    if (it->second.sold >= it->second.capacity) return std::nullopt;
    TicketId tid = next_ticket_id_++;
    Ticket t;
    t.id = tid; t.event = id; t.owner = buyer;
    t.paid = it->second.price;
    t.bought_at = (int64_t)std::time(nullptr);
    tickets_[tid] = t;
    owned_[buyer]++;
    it->second.sold++;
    return tid;
}
bool TicketSystem::CheckIn(TicketId id, const std::string& caller) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = tickets_.find(id);
    if (it == tickets_.end() || it->second.checked_in) return false;
    if (it->second.owner != caller) return false;
    it->second.checked_in = true;
    return true;
}
bool TicketSystem::Transfer(TicketId id, const std::string& from, const std::string& to) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = tickets_.find(id);
    if (it == tickets_.end() || it->second.owner != from) return false;
    if (it->second.checked_in) return false;
    if (to.empty() || to == from) return false;
    auto eit = events_.find(it->second.event);
    if (eit == events_.end() || !eit->second.transferable) return false;
    owned_[from]--;
    owned_[to]++;
    it->second.owner = to;
    return true;
}
std::optional<Ticket> TicketSystem::GetTicket(TicketId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = tickets_.find(id);
    if (it == tickets_.end()) return std::nullopt;
    return it->second;
}
uint64_t TicketSystem::TicketsOwnedBy(const std::string& o) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = owned_.find(o);
    return it == owned_.end() ? 0 : it->second;
}
size_t TicketSystem::EventCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return events_.size();
}
size_t TicketSystem::TicketCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return tickets_.size();
}
}
