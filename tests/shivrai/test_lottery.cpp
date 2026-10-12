#include "shivrai/lottery/lottery.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::lottery;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    Lottery l;
    CHECK(l.RoundCount() == 0);
    CHECK(!l.OpenRound(0).has_value());
    auto r1 = l.OpenRound(10);
    CHECK(r1.has_value());
    CHECK(l.RoundCount() == 1);
    CHECK(l.GetRound(*r1)->ticket_price == 10);
    CHECK(l.GetRound(*r1)->total_tickets == 0);
    CHECK(!l.BuyTickets(*r1, "", 1));
    CHECK(!l.BuyTickets(*r1, "alice", 0));
    CHECK(!l.BuyTickets(99999, "alice", 1));
    CHECK(l.BuyTickets(*r1, "alice", 5));
    CHECK(l.BuyTickets(*r1, "bob", 3));
    CHECK(l.TicketsOf(*r1, "alice") == 5);
    CHECK(l.TicketsOf(*r1, "bob") == 3);
    CHECK(l.TicketsOf(*r1, "carol") == 0);
    CHECK(l.GetRound(*r1)->total_tickets == 8);
    CHECK(l.GetRound(*r1)->jackpot == 80);
    CHECK(l.CloseRound(*r1));
    CHECK(!l.CloseRound(*r1));
    CHECK(!l.BuyTickets(*r1, "carol", 1));
    CHECK(l.DrawWinner(*r1, 0));
    CHECK(l.GetRound(*r1)->drawn);
    CHECK(!l.GetRound(*r1)->winner.empty());
    CHECK(!l.DrawWinner(*r1, 1));
    auto r2 = l.OpenRound(100);
    CHECK(r2.has_value());
    CHECK(!l.DrawWinner(*r2, 0));   // no tickets
    CHECK(!l.GetRound(99999).has_value());
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
