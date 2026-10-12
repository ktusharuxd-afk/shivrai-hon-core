#include "shivrai/governance/governance.h"
#include <cassert>
#include <iostream>

using namespace shivrai::governance;

int main() {
    int passed = 0;
    auto CHECK = [&](bool c) { assert(c); ++passed; };

    Governance gov;

    // Shorten periods so tests are deterministic (no sleeps)
    gov.SetVotingPeriodSec(0);
    gov.SetTimelockSec(0);

    CHECK(gov.VotingPeriodSec() == 0);
    CHECK(gov.TimelockSec() == 0);
    CHECK(gov.QuorumBps() == 4000);
    CHECK(gov.PassThresholdBps() == 5000);
    CHECK(gov.ProposalCount() == 0);
    CHECK(gov.VoterCount() == 0);
    CHECK(gov.TotalVotingPower() == 0);

    // --- Voting power registry ---
    gov.SetVotingPower("alice", 5000);
    gov.SetVotingPower("bob",   3000);
    gov.SetVotingPower("carol", 2000);
    CHECK(gov.VoterCount() == 3);
    CHECK(gov.TotalVotingPower() == 10000);
    CHECK(gov.VotingPowerOf("alice") == 5000);
    CHECK(gov.VotingPowerOf("bob")   == 3000);
    CHECK(gov.VotingPowerOf("carol") == 2000);
    CHECK(gov.VotingPowerOf("dave")  == 0);

    // Zero power removes voter
    gov.SetVotingPower("dave", 0);
    CHECK(gov.VoterCount() == 3);
    gov.SetVotingPower("dave", 1000);
    CHECK(gov.VoterCount() == 4);
    gov.SetVotingPower("dave", 0);
    CHECK(gov.VoterCount() == 3);

    // --- Config updates ---
    gov.SetQuorumBps(6000);
    CHECK(gov.QuorumBps() == 6000);
    gov.SetQuorumBps(4000);   // reset

    gov.SetPassThresholdBps(7000);
    CHECK(gov.PassThresholdBps() == 7000);
    gov.SetPassThresholdBps(5000);

    gov.SetQuorumBps(20000);  // invalid → ignored
    CHECK(gov.QuorumBps() == 4000);

    // --- Proposal validation ---
    CHECK(!gov.Propose("", "T", "D").has_value());
    CHECK(!gov.Propose("alice", "", "D").has_value());
    CHECK(!gov.Propose("nopower", "T", "D").has_value());

    // === Proposal 1: passes ===
    auto p1 = gov.Propose("alice", "Upgrade v2", "Do the upgrade");
    CHECK(p1.has_value());
    CHECK(gov.ProposalCount() == 1);

    auto pr1 = gov.GetProposal(*p1);
    CHECK(pr1.has_value());
    CHECK(pr1->state == ProposalState::Draft);
    CHECK(pr1->proposer == "alice");
    CHECK(pr1->title == "Upgrade v2");
    CHECK(pr1->snapshot_total_power == 0);

    CHECK(gov.ActivateProposal(*p1));
    auto pr1b = gov.GetProposal(*p1);
    CHECK(pr1b->state == ProposalState::Active);
    CHECK(pr1b->snapshot_total_power == 10000);
    CHECK(pr1b->voting_ends == 0 || pr1b->voting_ends >= pr1b->voting_starts);

    // Cannot activate twice
    CHECK(!gov.ActivateProposal(*p1));

    // Votes
    CHECK(gov.Vote(*p1, "alice", VoteType::For));
    CHECK(gov.Vote(*p1, "bob",   VoteType::Against));
    CHECK(gov.Vote(*p1, "carol", VoteType::Abstain));

    // Cannot double-vote
    CHECK(!gov.Vote(*p1, "alice", VoteType::Against));
    // No-power cannot vote
    CHECK(!gov.Vote(*p1, "nopower", VoteType::For));

    auto vrec = gov.GetVote(*p1, "alice");
    CHECK(vrec.has_value());
    CHECK(vrec->voter == "alice");
    CHECK(vrec->type == VoteType::For);
    CHECK(vrec->power == 5000);

    auto pr1c = gov.GetProposal(*p1);
    CHECK(pr1c->votes_for == 5000);
    CHECK(pr1c->votes_against == 3000);
    CHECK(pr1c->votes_abstain == 2000);

    // Tally
    auto t1 = gov.Tally(*p1);
    CHECK(t1.total_votes == 10000);
    CHECK(t1.quorum_pct_bps == 10000);
    CHECK(t1.quorum_met);
    CHECK(t1.for_pct_bps == 6250);  // 5000/8000
    CHECK(t1.passed);

    // Finalize
    CHECK(gov.FinalizeProposal(*p1));
    CHECK(gov.GetProposal(*p1)->state == ProposalState::Succeeded);
    CHECK(!gov.FinalizeProposal(*p1));  // already finalized

    // Queue + Execute
    CHECK(gov.QueueProposal(*p1));
    CHECK(gov.GetProposal(*p1)->state == ProposalState::Queued);
    CHECK(gov.ExecuteProposal(*p1));
    CHECK(gov.GetProposal(*p1)->state == ProposalState::Executed);

    // === Proposal 2: defeated (majority against) ===
    auto p2 = gov.Propose("alice", "Bad idea", "Nope");
    CHECK(p2.has_value());
    CHECK(gov.ActivateProposal(*p2));
    CHECK(gov.Vote(*p2, "alice", VoteType::Against));
    CHECK(gov.Vote(*p2, "bob",   VoteType::For));
    CHECK(gov.FinalizeProposal(*p2));
    CHECK(gov.GetProposal(*p2)->state == ProposalState::Defeated);

    auto t2 = gov.Tally(*p2);
    // for=3000, against=5000, total=8000, quorum=80%
    CHECK(t2.for_pct_bps == 3750);
    CHECK(t2.quorum_met);
    CHECK(!t2.passed);

    // Queue fails on Defeated
    CHECK(!gov.QueueProposal(*p2));

    // === Proposal 3: quorum not met ===
    auto p3 = gov.Propose("carol", "Small vote", "Low turnout");
    CHECK(p3.has_value());
    CHECK(gov.ActivateProposal(*p3));
    CHECK(gov.Vote(*p3, "carol", VoteType::For));
    CHECK(gov.FinalizeProposal(*p3));
    CHECK(gov.GetProposal(*p3)->state == ProposalState::Defeated);

    auto t3 = gov.Tally(*p3);
    // for=2000, total=2000, quorum=20% < 40%
    CHECK(t3.quorum_pct_bps == 2000);
    CHECK(!t3.quorum_met);
    CHECK(!t3.passed);

    // === Proposal 4: cancel ===
    auto p4 = gov.Propose("bob", "Scratch", "Will cancel");
    CHECK(p4.has_value());
    CHECK(!gov.CancelProposal("carol", *p4));   // wrong proposer
    CHECK(gov.CancelProposal("bob", *p4));
    CHECK(gov.GetProposal(*p4)->state == ProposalState::Cancelled);
    CHECK(!gov.CancelProposal("bob", *p4));     // already cancelled

    // === State queries ===
    auto executed = gov.ProposalsByState(ProposalState::Executed);
    CHECK(executed.size() == 1);
    CHECK(executed[0] == *p1);

    auto defeated = gov.ProposalsByState(ProposalState::Defeated);
    CHECK(defeated.size() == 2);

    auto all = gov.AllProposals();
    CHECK(all.size() == 4);

    // === Unknown IDs ===
    CHECK(!gov.GetProposal(99999).has_value());
    CHECK(!gov.ActivateProposal(99999));
    CHECK(!gov.Vote(99999, "alice", VoteType::For));
    CHECK(!gov.GetVote(*p1, "nopower").has_value());

    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}