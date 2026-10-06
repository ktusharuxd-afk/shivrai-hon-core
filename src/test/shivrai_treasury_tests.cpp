#include "shivrai/treasury/treasury.h"

#include <cassert>
#include <iostream>

using namespace shivrai::treasury;

int main()
{
    FeeAllocation a;
    assert(a.valid());
    const auto split = AllocateFee(1000, a);
    assert(split.security == 500);
    assert(split.burn == 200);
    assert(split.ecosystem == 150);
    assert(split.developer == 100);
    assert(split.reserve == 50);
    assert(split.accounted == 1000);

    TreasuryLedger ledger(a);
    assert(ledger.RecordNetworkFee(12345));
    assert(ledger.balance().total_controlled() + ledger.balance().burned == 12345);

    MultisigTreasury ms({"alice", "bob", "carol", "dave", "erin"}, 3, 10);
    assert(ms.valid());
    TreasuryProposal p;
    p.id = "proposal-1";
    p.destination = "hon1treasurydestination";
    p.amount = 500;
    p.created_height = 100;
    p.timelock.queued_at_height = 100;
    p.timelock.delay_blocks = ms.timelock_blocks();
    assert(ms.approve(p, "alice"));
    assert(ms.approve(p, "bob"));
    assert(ms.approve(p, "carol"));
    assert(ms.approval_count(p) == 3);
    assert(!ms.executable(p, 109));
    assert(ms.executable(p, 110));
    assert(!ms.approve(p, "mallory"));

    std::cout << "SHIVRAI treasury tests: PASS\n";
    return 0;
}
