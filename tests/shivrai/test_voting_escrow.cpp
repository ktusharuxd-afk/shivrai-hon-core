#include "shivrai/voting_escrow/voting_escrow.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::voting_escrow;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    VotingEscrow ve;
    CHECK(ve.LockCount() == 0);
    CHECK(ve.TotalLocked() == 0);
    CHECK(!ve.Lock("", 100, 100).has_value());
    CHECK(!ve.Lock("alice", 0, 100).has_value());
    CHECK(!ve.Lock("alice", 100, 0).has_value());
    CHECK(!ve.Lock("alice", 100, MAX_LOCK_SEC + 1).has_value());
    auto l1 = ve.Lock("alice", 1000, MAX_LOCK_SEC);
    CHECK(l1.has_value());
    CHECK(ve.LockCount() == 1);
    CHECK(ve.Get(*l1)->owner == "alice");
    CHECK(ve.Get(*l1)->amount == 1000);
    CHECK(ve.LocksOf("alice").size() == 1);
    CHECK(ve.TotalLocked() == 1000);
    // At max lock, voting power ≈ amount
    uint64_t vp = ve.VotingPower(*l1);
    CHECK(vp > 0);
    CHECK(ve.TotalVotingPowerOf("alice") >= vp - 1);
    CHECK(ve.TotalVotingPower() >= vp - 1);
    CHECK(ve.IncreaseAmount(*l1, "alice", 500));
    CHECK(ve.Get(*l1)->amount == 1500);
    CHECK(!ve.IncreaseAmount(*l1, "bob", 100));
    CHECK(!ve.IncreaseAmount(*l1, "alice", 0));
    CHECK(ve.ExtendLock(*l1, "alice", MAX_LOCK_SEC / 2));
    CHECK(!ve.ExtendLock(*l1, "bob", 100));
    CHECK(!ve.ExtendLock(*l1, "alice", 0));
    CHECK(!ve.Withdraw(*l1, "alice"));  // still locked
    CHECK(!ve.Withdraw(*l1, "bob"));
    CHECK(!ve.Get(99999).has_value());
    auto l2 = ve.Lock("bob", 2000, MAX_LOCK_SEC);
    CHECK(l2.has_value());
    CHECK(ve.TotalLocked() == 3500);
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
