#include "lottery.h"
#include <ctime>
namespace shivrai::lottery {
Lottery::Lottery() = default;
std::optional<RoundId> Lottery::OpenRound(uint64_t tp) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (tp == 0) return std::nullopt;
    RoundId id = next_id_++;
    Round r;
    r.id = id; r.ticket_price = tp;
    r.opened_at = (int64_t)std::time(nullptr);
    rounds_[id] = r;
    return id;
}
bool Lottery::BuyTickets(RoundId id, const std::string& buyer, uint64_t count) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = rounds_.find(id);
    if (it == rounds_.end() || buyer.empty() || count == 0) return false;
    if (it->second.closed_at != 0) return false;
    it->second.total_tickets += count;
    it->second.jackpot += it->second.ticket_price * count;
    tickets_[id][buyer] += count;
    for (uint64_t i = 0; i < count; ++i) ticket_order_[id].push_back(buyer);
    return true;
}
bool Lottery::CloseRound(RoundId id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = rounds_.find(id);
    if (it == rounds_.end() || it->second.closed_at != 0) return false;
    it->second.closed_at = (int64_t)std::time(nullptr);
    return true;
}
bool Lottery::DrawWinner(RoundId id, uint64_t seed) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = rounds_.find(id);
    if (it == rounds_.end() || it->second.drawn) return false;
    if (it->second.total_tickets == 0) return false;
    auto& order = ticket_order_[id];
    uint64_t idx = seed % order.size();
    it->second.winner = order[idx];
    it->second.drawn = true;
    return true;
}
std::optional<Round> Lottery::GetRound(RoundId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = rounds_.find(id);
    if (it == rounds_.end()) return std::nullopt;
    return it->second;
}
uint64_t Lottery::TicketsOf(RoundId id, const std::string& buyer) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = tickets_.find(id);
    if (it == tickets_.end()) return 0;
    auto uit = it->second.find(buyer);
    return uit == it->second.end() ? 0 : uit->second;
}
size_t Lottery::RoundCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return rounds_.size();
}
}
