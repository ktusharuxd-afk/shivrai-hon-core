#ifndef SHIVRAI_HON_GOVERNANCE_H
#define SHIVRAI_HON_GOVERNANCE_H

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace shivrai::governance {

using ProposalId = uint64_t;

static constexpr uint32_t BPS = 10000;
static constexpr uint32_t DEFAULT_QUORUM_BPS = 4000;      // 40% of total voting power
static constexpr uint32_t DEFAULT_PASS_THRESHOLD_BPS = 5000;  // >50% of for/(for+against)
static constexpr int64_t  DEFAULT_VOTING_PERIOD_SEC = 3 * 24 * 3600;
static constexpr int64_t  DEFAULT_TIMELOCK_SEC = 2 * 24 * 3600;

enum class ProposalState {
    Draft,
    Active,
    Defeated,
    Succeeded,
    Queued,
    Executed,
    Cancelled
};

enum class VoteType { Against = 0, For = 1, Abstain = 2 };

struct Proposal {
    ProposalId   id{0};
    std::string  proposer;
    std::string  title;
    std::string  description;
    int64_t      created_at{0};
    int64_t      voting_starts{0};
    int64_t      voting_ends{0};
    int64_t      queued_at{0};
    int64_t      eta{0};                 // earliest execution time
    int64_t      executed_at{0};
    ProposalState state{ProposalState::Draft};

    uint64_t     votes_for{0};
    uint64_t     votes_against{0};
    uint64_t     votes_abstain{0};

    uint32_t     quorum_bps{DEFAULT_QUORUM_BPS};
    uint32_t     pass_threshold_bps{DEFAULT_PASS_THRESHOLD_BPS};
    uint64_t     snapshot_total_power{0};
};

struct VoteRecord {
    std::string voter;
    VoteType    type{VoteType::Abstain};
    uint64_t    power{0};
    int64_t     cast_at{0};
};

struct ProposalResult {
    bool     passed{false};
    bool     quorum_met{false};
    uint64_t total_votes{0};
    uint64_t for_pct_bps{0};        // of for+against
    uint64_t quorum_pct_bps{0};     // of snapshot
};

class Governance {
public:
    Governance();

    // --- Config ---
    void SetQuorumBps(uint32_t bps);
    void SetPassThresholdBps(uint32_t bps);
    void SetVotingPeriodSec(int64_t sec);
    void SetTimelockSec(int64_t sec);
    uint32_t QuorumBps() const;
    uint32_t PassThresholdBps() const;
    int64_t  VotingPeriodSec() const;
    int64_t  TimelockSec() const;

    // --- Voting power registry (called by staking/token layer) ---
    void SetVotingPower(const std::string& voter, uint64_t power);
    uint64_t VotingPowerOf(const std::string& voter) const;
    uint64_t TotalVotingPower() const;
    size_t   VoterCount() const;

    // --- Proposals ---
    std::optional<ProposalId> Propose(const std::string& proposer,
                                      const std::string& title,
                                      const std::string& description);
    bool ActivateProposal(ProposalId id);
    bool CancelProposal(const std::string& proposer, ProposalId id);

    // --- Voting ---
    bool Vote(ProposalId id, const std::string& voter, VoteType type);

    // --- Finalization ---
    ProposalResult Tally(ProposalId id) const;
    bool FinalizeProposal(ProposalId id);      // Active → Succeeded / Defeated

    // --- Queue & Execute ---
    bool QueueProposal(ProposalId id);
    bool ExecuteProposal(ProposalId id);

    // --- Queries ---
    std::optional<Proposal> GetProposal(ProposalId id) const;
    std::vector<ProposalId> AllProposals() const;
    std::vector<ProposalId> ProposalsByState(ProposalState s) const;
    std::optional<VoteRecord> GetVote(ProposalId id, const std::string& voter) const;
    size_t ProposalCount() const;

private:
    mutable std::recursive_mutex cs_;
    std::map<ProposalId, Proposal> proposals_;
    std::map<ProposalId, std::map<std::string, VoteRecord>> votes_;
    std::map<std::string, uint64_t> voting_power_;

    ProposalId next_proposal_id_{1};
    uint32_t   quorum_bps_{DEFAULT_QUORUM_BPS};
    uint32_t   pass_threshold_bps_{DEFAULT_PASS_THRESHOLD_BPS};
    int64_t    voting_period_sec_{DEFAULT_VOTING_PERIOD_SEC};
    int64_t    timelock_sec_{DEFAULT_TIMELOCK_SEC};
};

}
#endif