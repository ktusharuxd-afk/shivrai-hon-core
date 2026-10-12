#include "shivrai/ticket_system/ticket_system.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::ticket_system;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    TicketSystem ts;
    CHECK(ts.EventCount() == 0);
    CHECK(ts.TicketCount() == 0);
    CHECK(!ts.CreateEvent("", 100, 50, 0).has_value());
    CHECK(!ts.CreateEvent("Concert", 0, 50, 0).has_value());
    CHECK(!ts.CreateEvent("Concert", 100, 0, 0).has_value());
    auto e1 = ts.CreateEvent("Concert", 3, 100, 9999999999LL);
    CHECK(e1.has_value());
    CHECK(ts.EventCount() == 1);
    CHECK(ts.GetEvent(*e1)->name == "Concert");
    CHECK(ts.GetEvent(*e1)->capacity == 3);
    CHECK(ts.GetEvent(*e1)->price == 100);
    CHECK(ts.GetEvent(*e1)->sold == 0);
    CHECK(ts.GetEvent(*e1)->active);
    CHECK(ts.GetEvent(*e1)->transferable);
    CHECK(!ts.Buy(*e1, "").has_value());
    CHECK(!ts.Buy(99999, "alice").has_value());
    auto t1 = ts.Buy(*e1, "alice");
    auto t2 = ts.Buy(*e1, "alice");
    auto t3 = ts.Buy(*e1, "bob");
    CHECK(t1.has_value() && t2.has_value() && t3.has_value());
    CHECK(ts.TicketCount() == 3);
    CHECK(ts.GetEvent(*e1)->sold == 3);
    CHECK(ts.TicketsOwnedBy("alice") == 2);
    CHECK(ts.TicketsOwnedBy("bob") == 1);
    CHECK(ts.TicketsOwnedBy("carol") == 0);
    CHECK(!ts.Buy(*e1, "carol").has_value()); // sold out
    auto tk = ts.GetTicket(*t1);
    CHECK(tk.has_value());
    CHECK(tk->owner == "alice");
    CHECK(tk->paid == 100);
    CHECK(!tk->checked_in);
    CHECK(!ts.CheckIn(*t1, "bob"));
    CHECK(ts.CheckIn(*t1, "alice"));
    CHECK(ts.GetTicket(*t1)->checked_in);
    CHECK(!ts.CheckIn(*t1, "alice"));
    CHECK(!ts.Transfer(*t1, "alice", "carol"));  // checked in
    CHECK(ts.Transfer(*t2, "alice", "carol"));
    CHECK(ts.GetTicket(*t2)->owner == "carol");
    CHECK(ts.TicketsOwnedBy("alice") == 1);
    CHECK(ts.TicketsOwnedBy("carol") == 1);
    CHECK(!ts.Transfer(*t2, "alice", "dave"));   // not owner
    CHECK(!ts.Transfer(*t2, "carol", "carol"));  // self
    CHECK(!ts.Transfer(*t2, "carol", ""));       // empty
    ts.SetTransferable(*e1, false);
    CHECK(!ts.Transfer(*t3, "bob", "dave"));
    ts.SetActive(*e1, false);
    CHECK(!ts.Buy(*e1, "dave").has_value());
    CHECK(!ts.SetActive(99999, true));
    CHECK(!ts.SetTransferable(99999, true));
    CHECK(!ts.GetTicket(99999).has_value());
    auto e2 = ts.CreateEvent("Movie", 5, 20, 0);
    CHECK(e2.has_value());
    CHECK(ts.EventCount() == 2);
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
