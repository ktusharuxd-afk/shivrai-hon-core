#include "shivrai/multisig/multisig.h"
#include <cstdlib>
#include <iostream>

using namespace shivrai::multisig;

int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL line " << __LINE__ << ": " #x "\n"; std::exit(1); } } while(0)

    Multisig ms;

    CHECK(ms.WalletCount() == 0);
    CHECK(ms.TransactionCount() == 0);

    // ---- Wallet creation ----
    auto w1 = ms.CreateWallet("Treasury", {"alice","bob","carol"}, 2);
    CHECK(w1.has_value());
    CHECK(ms.WalletCount() == 1);

    auto w = ms.GetWallet(*w1);
    CHECK(w.has_value());
    CHECK(w->name == "Treasury");
    CHECK(w->threshold == 2);
    CHECK(w->owners.size() == 3);
    CHECK(w->balance == 0);

    // Invalid inputs
    CHECK(!ms.CreateWallet("", {"a","b"}, 2).has_value());
    CHECK(!ms.CreateWallet("X", {}, 1).has_value());
    CHECK(!ms.CreateWallet("X", {"a","b"}, 0).has_value());
    CHECK(!ms.CreateWallet("X", {"a","b"}, 3).has_value());
    CHECK(!ms.CreateWallet("X", {"a","a"}, 1).has_value());  // dup owner
    CHECK(!ms.CreateWallet("X", {"", "b"}, 1).has_value());  // empty owner

    // ---- Owner queries ----
    CHECK(ms.IsOwner(*w1, "alice"));
    CHECK(ms.IsOwner(*w1, "bob"));
    CHECK(ms.IsOwner(*w1, "carol"));
    CHECK(!ms.IsOwner(*w1, "dave"));
    CHECK(!ms.IsOwner(99999, "alice"));

    CHECK(ms.Threshold(*w1) == 2);
    CHECK(ms.Threshold(99999) == 0);

    auto owners = ms.Owners(*w1);
    CHECK(owners.size() == 3);

    // ---- Deposit ----
    CHECK(ms.Deposit(*w1, 10000));
    CHECK(ms.Balance(*w1) == 10000);
    CHECK(!ms.Deposit(*w1, 0));
    CHECK(!ms.Deposit(99999, 100));

    // ---- Propose tx ----
    auto t1 = ms.Propose(*w1, "alice", "vendor", 1000, "payment");
    CHECK(t1.has_value());
    CHECK(ms.TransactionCount() == 1);

    auto tx = ms.GetTransaction(*t1);
    CHECK(tx.has_value());
    CHECK(tx->proposer == "alice");
    CHECK(tx->to == "vendor");
    CHECK(tx->amount == 1000);
    CHECK(tx->memo == "payment");
    CHECK(tx->state == TxState::Pending);
    CHECK(tx->approvals.size() == 1);       // proposer auto-approved
    CHECK(tx->nonce == 1);

    // Invalid proposals
    CHECK(!ms.Propose(*w1, "dave", "x", 100, "").has_value());     // not owner
    CHECK(!ms.Propose(*w1, "alice", "", 100, "").has_value());     // no to
    CHECK(!ms.Propose(*w1, "alice", "x", 0, "").has_value());      // zero amount
    CHECK(!ms.Propose(*w1, "alice", "x", 99999999, "").has_value());// over balance
    CHECK(!ms.Propose(99999, "alice", "x", 100, "").has_value());  // bad wallet

    CHECK(ms.ApprovalCount(*t1) == 1);
    CHECK(ms.HasApproved(*t1, "alice"));
    CHECK(!ms.HasApproved(*t1, "bob"));

    // ---- Approvals ----
    CHECK(ms.Approve(*w1, *t1, "bob"));
    CHECK(ms.ApprovalCount(*t1) == 2);
    CHECK(ms.HasApproved(*t1, "bob"));

    // Double approve fails
    CHECK(!ms.Approve(*w1, *t1, "bob"));
    // Non-owner approve fails
    CHECK(!ms.Approve(*w1, *t1, "dave"));

    // ---- Execute ----
    // threshold = 2, has 2 approvals → execute
    uint64_t bal_before = ms.Balance(*w1);
    CHECK(ms.Execute(*w1, *t1, "alice"));
    CHECK(ms.Balance(*w1) == bal_before - 1000);
    CHECK(ms.GetTransaction(*t1)->state == TxState::Executed);

    // Double execute fails
    CHECK(!ms.Execute(*w1, *t1, "alice"));

    // ---- Nonce bumped ----
    CHECK(ms.GetWallet(*w1)->nonce == 1);

    // ---- Propose second tx — needs 2 approvals again ----
    auto t2 = ms.Propose(*w1, "bob", "charity", 500, "donation");
    CHECK(t2.has_value());
    CHECK(ms.GetTransaction(*t2)->nonce == 2);

    // Execute with only 1 approval fails
    CHECK(!ms.Execute(*w1, *t2, "bob"));

    // Carol approves → 2 approvals → execute
    CHECK(ms.Approve(*w1, *t2, "carol"));
    CHECK(ms.Execute(*w1, *t2, "carol"));
    CHECK(ms.Balance(*w1) == bal_before - 1000 - 500);

    // ---- Revoke approval ----
    auto t3 = ms.Propose(*w1, "alice", "x", 100, "");
    CHECK(t3.has_value());
    CHECK(ms.Approve(*w1, *t3, "bob"));
    CHECK(ms.ApprovalCount(*t3) == 2);
    CHECK(ms.RevokeApproval(*w1, *t3, "bob"));
    CHECK(ms.ApprovalCount(*t3) == 1);
    CHECK(!ms.RevokeApproval(*w1, *t3, "alice"));   // proposer cannot revoke
    CHECK(!ms.RevokeApproval(*w1, *t3, "dave"));    // not owner
    CHECK(!ms.Execute(*w1, *t3, "alice"));          // only 1 approval now

    // ---- Cancel (by proposer) ----
    CHECK(!ms.Cancel(*w1, *t3, "bob"));             // not proposer
    CHECK(ms.Cancel(*w1, *t3, "alice"));
    CHECK(ms.GetTransaction(*t3)->state == TxState::Cancelled);
    CHECK(!ms.Cancel(*w1, *t3, "alice"));           // already cancelled

    // ---- Pending txs ----
    auto t4 = ms.Propose(*w1, "bob", "y", 200, "");
    CHECK(t4.has_value());
    auto pending = ms.PendingTxs(*w1);
    CHECK(pending.size() == 1);
    CHECK(pending[0] == *t4);

    CHECK(ms.PendingTxs(99999).empty());

    // ---- Owner management ----
    CHECK(ms.AddOwner(*w1, "alice", "dave"));
    CHECK(ms.Owners(*w1).size() == 4);
    CHECK(ms.IsOwner(*w1, "dave"));
    CHECK(!ms.AddOwner(*w1, "alice", "dave"));      // dup
    CHECK(ms.AddOwner(*w1, "dave", "eve"));         // dave is owner → can add eve
    CHECK(ms.IsOwner(*w1, "eve"));

    CHECK(!ms.AddOwner(*w1, "nobody", "z"));
    CHECK(!ms.AddOwner(*w1, "alice", ""));

    // ---- Change threshold ----
    CHECK(ms.ChangeThreshold(*w1, "alice", 3));
    CHECK(ms.Threshold(*w1) == 3);
    CHECK(!ms.ChangeThreshold(*w1, "alice", 0));
    CHECK(!ms.ChangeThreshold(*w1, "alice", 10));
    CHECK(!ms.ChangeThreshold(*w1, "nobody", 2));

    // ---- Remove owner ----
    CHECK(ms.RemoveOwner(*w1, "alice", "eve"));
    CHECK(!ms.IsOwner(*w1, "eve"));
    CHECK(!ms.RemoveOwner(*w1, "alice", "nobody"));
    CHECK(!ms.RemoveOwner(*w1, "nobody", "alice"));

    // threshold is 3 now, owners = alice, bob, carol, dave (4)
    // Remove one — try removing bob: owners would be 3 → threshold 3 still OK
    CHECK(ms.RemoveOwner(*w1, "alice", "bob"));
    CHECK(ms.Owners(*w1).size() == 3);

    // Now try to remove another — threshold 3 > 2 remaining → should fail
    CHECK(!ms.RemoveOwner(*w1, "alice", "carol"));

    // ---- Unknown tx lookups ----
    CHECK(!ms.GetTransaction(99999).has_value());
    CHECK(ms.ApprovalCount(99999) == 0);
    CHECK(!ms.HasApproved(99999, "alice"));
    CHECK(!ms.Approve(*w1, 99999, "alice"));
    CHECK(!ms.Execute(*w1, 99999, "alice"));
    CHECK(!ms.Cancel(*w1, 99999, "alice"));
    CHECK(!ms.RevokeApproval(*w1, 99999, "alice"));

    // Wrong wallet id in approve
    auto w2 = ms.CreateWallet("Second", {"x","y"}, 1);
    CHECK(w2.has_value());
    CHECK(!ms.Approve(*w2, *t4, "alice"));          // t4 belongs to w1
    CHECK(!ms.Execute(*w2, *t4, "x"));              // t4 not in w2

    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}