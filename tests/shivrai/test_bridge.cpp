#include "shivrai/bridge/bridge.h"
#include <cstdlib>
#include <iostream>

using namespace shivrai::bridge;

int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL line " << __LINE__ << ": " #x "\n"; std::exit(1); } } while(0)

    Bridge b;

    // Short challenge period for tests
    b.SetChallengePeriodSec(0);

    // ---- Config defaults ----
    CHECK(b.BridgeFeeBps() == 10);
    CHECK(b.ValidatorThresholdBps() == 6667);
    CHECK(b.ChallengePeriodSec() == 0);
    CHECK(b.ChainCount() == 0);
    CHECK(b.ValidatorCount() == 0);
    CHECK(b.TransferCount() == 0);
    CHECK(b.TotalValidatorPower() == 0);

    // ---- Chain registry ----
    CHECK(b.RegisterChain("eth", 12));
    CHECK(b.RegisterChain("shivrai", 1));
    CHECK(b.RegisterChain("polygon", 30));
    CHECK(b.ChainCount() == 3);
    CHECK(!b.RegisterChain("eth", 12));      // dup
    CHECK(!b.RegisterChain("", 1));          // empty

    auto eth = b.GetChain("eth");
    CHECK(eth.has_value());
    CHECK(eth->chain_id == "eth");
    CHECK(eth->confirmations_required == 12);
    CHECK(eth->active);
    CHECK(!b.GetChain("nope").has_value());

    CHECK(b.ActiveChains().size() == 3);

    CHECK(b.SetChainActive("polygon", false));
    CHECK(!b.GetChain("polygon")->active);
    CHECK(b.ActiveChains().size() == 2);
    CHECK(!b.SetChainActive("nope", false));

    // ---- Validators ----
    CHECK(b.AddValidator("val1", 100));
    CHECK(b.AddValidator("val2", 100));
    CHECK(b.AddValidator("val3", 50));
    CHECK(b.ValidatorCount() == 3);
    CHECK(b.TotalValidatorPower() == 250);
    CHECK(b.ValidatorPower("val1") == 100);
    CHECK(b.ValidatorPower("nope") == 0);

    CHECK(!b.AddValidator("val1", 100));    // dup
    CHECK(!b.AddValidator("", 100));        // empty
    CHECK(!b.AddValidator("val4", 0));      // zero power

    // ---- Lock validation ----
    CHECK(!b.Lock("", "shivrai", "alice", "bob", "USDC", 1000, "0xabc").has_value());
    CHECK(!b.Lock("eth", "eth", "alice", "bob", "USDC", 1000, "0xabc").has_value());  // same chain
    CHECK(!b.Lock("eth", "shivrai", "", "bob", "USDC", 1000, "0xabc").has_value());
    CHECK(!b.Lock("eth", "shivrai", "alice", "", "USDC", 1000, "0xabc").has_value());
    CHECK(!b.Lock("eth", "shivrai", "alice", "bob", "", 1000, "0xabc").has_value());
    CHECK(!b.Lock("eth", "shivrai", "alice", "bob", "USDC", 50, "0xabc").has_value());  // below min
    CHECK(!b.Lock("eth", "shivrai", "alice", "bob", "USDC", 1000, "").has_value());     // no hash
    CHECK(!b.Lock("nope", "shivrai", "alice", "bob", "USDC", 1000, "0xabc").has_value());
    CHECK(!b.Lock("eth", "polygon", "alice", "bob", "USDC", 1000, "0xabc").has_value()); // polygon inactive

    // ---- Successful lock: 10000 USDC ----
    auto t1 = b.Lock("eth", "shivrai", "alice", "bob", "USDC", 10000, "0xhash1");
    CHECK(t1.has_value());
    CHECK(b.TransferCount() == 1);

    auto tx1 = b.GetTransfer(*t1);
    CHECK(tx1.has_value());
    CHECK(tx1->from_chain == "eth");
    CHECK(tx1->to_chain == "shivrai");
    CHECK(tx1->sender == "alice");
    CHECK(tx1->recipient == "bob");
    CHECK(tx1->asset == "USDC");
    CHECK(tx1->amount == 10000);
    CHECK(tx1->fee == 10);              // 0.1%
    CHECK(tx1->net_amount == 9990);
    CHECK(tx1->state == TransferState::Pending);
    CHECK(tx1->attestations.empty());

    CHECK(b.TotalLocked("eth", "USDC") == 10000);
    CHECK(b.TotalLocked("eth", "ETH") == 0);
    CHECK(b.TotalLocked("shivrai", "USDC") == 0);

    // ---- Attestation ----
    CHECK(!b.Attest(*t1, "nope"));                // unknown validator
    CHECK(b.Attest(*t1, "val1"));
    CHECK(b.HasAttested(*t1, "val1"));
    CHECK(!b.HasAttested(*t1, "val2"));
    CHECK(!b.Attest(*t1, "val1"));                // dup

    CHECK(b.AttestedPower(*t1) == 100);           // val1's power

    auto tx1b = b.GetTransfer(*t1);
    CHECK(tx1b->state == TransferState::Attesting);
    CHECK(tx1b->attestations.size() == 1);

    // ---- MarkReady fails at 40% (100/250) ----
    CHECK(!b.MarkReady(*t1));

    // Add val2 → 200/250 = 80% > 66.67%
    CHECK(b.Attest(*t1, "val2"));
    CHECK(b.AttestedPower(*t1) == 200);
    CHECK(b.MarkReady(*t1));

    auto tx1c = b.GetTransfer(*t1);
    CHECK(tx1c->state == TransferState::Ready);
    CHECK(tx1c->ready_at > 0);

    // ---- Execute (0 challenge period, immediately allowed) ----
    CHECK(b.Execute(*t1));
    CHECK(b.GetTransfer(*t1)->state == TransferState::Executed);

    // Double execute fails
    CHECK(!b.Execute(*t1));

    // Cannot attest executed tx
    CHECK(!b.Attest(*t1, "val3"));
    CHECK(!b.RevokeAttestation(*t1, "val1"));

    // ---- Revoke attestation ----
    auto t2 = b.Lock("eth", "shivrai", "carol", "dave", "USDC", 5000, "0xhash2");
    CHECK(t2.has_value());
    CHECK(b.Attest(*t2, "val1"));
    CHECK(b.Attest(*t2, "val2"));
    CHECK(b.AttestedPower(*t2) == 200);

    CHECK(b.RevokeAttestation(*t2, "val2"));
    CHECK(b.AttestedPower(*t2) == 100);
    CHECK(b.GetTransfer(*t2)->state == TransferState::Attesting);

    CHECK(b.RevokeAttestation(*t2, "val1"));
    CHECK(b.GetTransfer(*t2)->state == TransferState::Pending);

    // ---- Threshold config ----
    b.SetValidatorThresholdBps(5000);    // 50%
    CHECK(b.ValidatorThresholdBps() == 5000);
    CHECK(b.Attest(*t2, "val1"));        // 100/250 = 40% — still below
    CHECK(!b.MarkReady(*t2));
    CHECK(b.Attest(*t2, "val3"));        // 150/250 = 60% — above 50%
    CHECK(b.MarkReady(*t2));

    b.SetValidatorThresholdBps(0);       // invalid — ignore
    CHECK(b.ValidatorThresholdBps() == 5000);
    b.SetValidatorThresholdBps(20000);   // invalid — ignore
    CHECK(b.ValidatorThresholdBps() == 5000);
    b.SetValidatorThresholdBps(6667);    // reset

    // ---- Challenge flow ----
    auto t3 = b.Lock("eth", "shivrai", "eve", "frank", "USDC", 2000, "0xhash3");
    CHECK(t3.has_value());
    CHECK(b.Attest(*t3, "val1"));
    CHECK(b.Attest(*t3, "val2"));
    CHECK(b.MarkReady(*t3));
    CHECK(b.GetTransfer(*t3)->state == TransferState::Ready);

    CHECK(!b.Challenge(99999, "watcher", "no such tx"));
    CHECK(b.Challenge(*t3, "watcher", "double-spend suspected"));
    CHECK(b.GetTransfer(*t3)->state == TransferState::Challenged);

    // Cannot double challenge
    CHECK(!b.Challenge(*t3, "watcher", "again"));

    // Cannot execute challenged
    CHECK(!b.Execute(*t3));

    // Resolve as not fraudulent → back to Attesting
    CHECK(b.ResolveChallenge(*t3, false));
    CHECK(b.GetTransfer(*t3)->state == TransferState::Attesting);

    // Re-ready and challenge again, resolve as fraudulent → Cancelled
    CHECK(b.MarkReady(*t3));
    CHECK(b.Challenge(*t3, "watcher", "still fraud"));
    CHECK(b.ResolveChallenge(*t3, true));
    CHECK(b.GetTransfer(*t3)->state == TransferState::Cancelled);

    // Cannot execute cancelled
    CHECK(!b.Execute(*t3));

    // ---- Cancel ----
    auto t4 = b.Lock("eth", "shivrai", "g", "h", "USDC", 300, "0xhash4");
    CHECK(t4.has_value());
    CHECK(b.Cancel(*t4, "user request"));
    CHECK(b.GetTransfer(*t4)->state == TransferState::Cancelled);
    CHECK(!b.Cancel(*t4, "again"));      // already cancelled

    // ---- Fee config ----
    b.SetBridgeFeeBps(50);               // 0.5%
    CHECK(b.BridgeFeeBps() == 50);
    auto t5 = b.Lock("eth", "shivrai", "i", "j", "USDC", 10000, "0xhash5");
    CHECK(t5.has_value());
    CHECK(b.GetTransfer(*t5)->fee == 50);
    CHECK(b.GetTransfer(*t5)->net_amount == 9950);

    b.SetBridgeFeeBps(20000);            // invalid
    CHECK(b.BridgeFeeBps() == 50);
    b.SetBridgeFeeBps(10);               // reset

    // ---- Validator removal ----
    CHECK(b.RemoveValidator("val3"));
    CHECK(b.ValidatorCount() == 2);
    CHECK(b.TotalValidatorPower() == 200);
    CHECK(!b.RemoveValidator("val3"));   // already removed

    // ---- Queries ----
    auto executed = b.TransfersByState(TransferState::Executed);
    CHECK(executed.size() == 1);
    CHECK(executed[0] == *t1);

    auto cancelled = b.TransfersByState(TransferState::Cancelled);
    CHECK(cancelled.size() == 2);        // t3, t4

    auto to_shivrai = b.TransfersTo("shivrai");
    CHECK(to_shivrai.size() == 5);

    auto to_none = b.TransfersTo("nonexistent");
    CHECK(to_none.empty());

    CHECK(!b.GetTransfer(99999).has_value());
    CHECK(b.AttestedPower(99999) == 0);
    CHECK(!b.HasAttested(99999, "val1"));
    CHECK(!b.Attest(99999, "val1"));
    CHECK(!b.MarkReady(99999));
    CHECK(!b.Execute(99999));
    CHECK(!b.RevokeAttestation(99999, "val1"));

    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}