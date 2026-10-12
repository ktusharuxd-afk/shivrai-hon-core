#ifndef SHIVRAI_HON_LOTTERY_H
#define SHIVRAI_HON_LOTTERY_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>
namespace shivrai::lottery {
using RoundId = uint64_t;
struct Ticket { std::string buyer; uint64_t count{0}; };
struct Round {
    RoundId id{0};
    uint64_t ticket_price{0};
    uint64_t total_tickets{0};
    uint64_t jackpot{0};
    int64_t opened_at{0};
    int64_t closed_at{0};
    std::string winner;
    bool drawn{false};
};
class Lottery {
public:
    Lottery();
    std::optional<RoundId> OpenRound(uint64_t ticket_price);
    bool BuyTickets(RoundId id, const std::string& buyer, uint64_t count);
    bool CloseRound(RoundId id);
    bool DrawWinner(RoundId id, uint64_t seed);
    std::optional<Round> GetRound(RoundId id) const;
    uint64_t TicketsOf(RoundId id, const std::string& buyer) const;
    size_t RoundCount() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<RoundId, Round> rounds_;
    std::map<RoundId, std::map<std::string, uint64_t>> tickets_;
    std::map<RoundId, std::vector<std::string>> ticket_order_;
    RoundId next_id_{1};
};
}
#endif
