#include "shivrai/staking/staking.h"
#include <cassert>
#include <iostream>

using namespace shivrai::staking;

int main() {
    int passed = 0;
    auto CHECK = [&](bool c) { assert(c); ++passed; };

    StakingPool sp;

    CHECK(sp.ValidatorCount() == 0);
    CHECK(sp.ValidatorCountActive() == 0);
    CHECK(sp.DelegationCount() == 0);
    CHECK(sp.TotalStaked() == 0);
    CHECK(sp.TotalBonded() == 0);
    CHECK(sp.TotalSlashed() == 0);

    // --- Validator registration ---
    CHECK(sp.RegisterValidator("Alpha", "val1", 10000, 500));
    CHECK(sp.RegisterValidator("Beta",  "val2", 20000, 1000));
    CHECK(sp.ValidatorCount() == 2);
    CHECK(sp.ValidatorCountActive() == 2);

    CHECK(!sp.RegisterValidator("Dup", "val1", 10000, 500));      // dup id
    CHECK(!sp.RegisterValidator("Low", "val3", 5000, 500));       // below min self
    CHECK(!sp.RegisterValidator("Bad", "val4", 10000, 20000));    // commission > 100%

    auto v1 = sp.GetValidator("val1");
    CHECK(v1.has_value());
    CHECK(v1->moniker == "Alpha");
    CHECK(v1->self_stake == 10000);
    CHECK(v1->total_stake == 10000);
    CHECK(v1->commission_bps == 500);
    CHECK(v1->status == ValidatorStatus::Active);

    CHECK(sp.TotalStaked() == 30000);
    CHECK(sp.TotalBonded() == 30000);

    // --- Commission update ---
    CHECK(sp.UpdateCommission("val1", 1000));
    CHECK(sp.GetValidator("val1")->commission_bps == 1000);
    CHECK(!sp.UpdateCommission("val1", 20000));
    CHECK(!sp.UpdateCommission("nope", 500));

    // --- Delegation ---
    auto d1 = sp.Delegate("alice", "val1", 5000);
    auto d2 = sp.Delegate("bob",   "val1", 3000);
    auto d3 = sp.Delegate("carol", "val2", 7000);
    CHECK(d1.has_value() && d2.has_value() && d3.has_value());
    CHECK(sp.DelegationCount() == 3);

    CHECK(!sp.Delegate("dave", "val1", 50).has_value());         // below min
    CHECK(!sp.Delegate("dave", "nope", 5000).has_value());       // bad validator

    auto v1b = sp.GetValidator("val1");
    CHECK(v1b->delegated == 8000);
    CHECK(v1b->total_stake == 18000);

    CHECK(sp.DelegationsOf("alice").size() == 1);
    CHECK(sp.DelegationsTo("val1").size() == 2);
    CHECK(sp.DelegatedBy("alice") == 5000);
    CHECK(sp.DelegatedBy("bob") == 3000);
    CHECK(sp.DelegatedBy("carol") == 7000);
    CHECK(sp.DelegatedBy("dave") == 0);

    auto dg = sp.GetDelegation(*d1);
    CHECK(dg.has_value());
    CHECK(dg->delegator == "alice");
    CHECK(dg->validator == "val1");
    CHECK(dg->amount == 5000);
    CHECK(!dg->unbonding);

    // --- Rewards: 1000 reward, val1 commission 10% ---
    sp.DistributeRewards("val1", 1000);
    // commission = 100
    // to_delegators = 900
    // alice 5000/8000 -> 562 (rounding: 900*5000/8000 = 562)
    // bob   3000/8000 -> remaining 338
    uint64_t a_pending = sp.PendingRewards("alice", *d1);
    uint64_t b_pending = sp.PendingRewards("bob",   *d2);
    CHECK(a_pending + b_pending == 900);
    CHECK(a_pending == 562);
    CHECK(b_pending == 338);

    CHECK(sp.PendingRewards("carol", *d3) == 0);
    CHECK(sp.PendingRewards("alice", *d2) == 0);  // wrong delegator

    uint64_t claimed = sp.ClaimRewards("alice", *d1);
    CHECK(claimed == 562);
    CHECK(sp.PendingRewards("alice", *d1) == 0);

    // --- Undelegate flow ---
    CHECK(sp.BeginUndelegate("alice", *d1));
    CHECK(sp.GetDelegation(*d1)->unbonding);
    CHECK(!sp.BeginUndelegate("alice", *d1));    // already unbonding
    CHECK(!sp.BeginUndelegate("bob", *d1));      // wrong owner

    CHECK(!sp.CompleteUndelegate(*d1));          // unbond period not passed

    // --- Slashing: double sign ---
    uint64_t total_before = sp.TotalStaked();
    CHECK(sp.SlashDoubleSign("val2"));
    // val2 = 20000 self + 7000 delegated = 27000; slash 5% = 1350
    CHECK(sp.TotalSlashed() == 1350);
    CHECK(sp.GetValidator("val2")->status == ValidatorStatus::Tombstoned);
    CHECK(sp.TotalStaked() == total_before - 1350);
    CHECK(!sp.SlashDoubleSign("val2"));          // already tombstoned

    // --- Downtime slash ---
    uint64_t t2 = sp.TotalSlashed();
    CHECK(sp.SlashDowntime("val1"));
    // val1 = 18000; slash 0.1% = 18
    CHECK(sp.TotalSlashed() == t2 + 18);
    CHECK(!sp.SlashDowntime("nope"));

    // --- Jail ---
    CHECK(sp.JailValidator("val1", 3600));
    CHECK(sp.GetValidator("val1")->status == ValidatorStatus::Jailed);
    CHECK(sp.ValidatorCountActive() == 0);       // val2 tombstoned, val1 jailed
    CHECK(!sp.UpdateCommission("val1", 800));    // cannot update while jailed
    CHECK(sp.UnjailValidator("val1"));
    CHECK(sp.GetValidator("val1")->status == ValidatorStatus::Active);
    CHECK(!sp.UnjailValidator("val1"));          // not jailed

    CHECK(!sp.JailValidator("nope", 100));

    // --- All validators listing ---
    auto all = sp.AllValidators();
    CHECK(all.size() == 2);

    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}