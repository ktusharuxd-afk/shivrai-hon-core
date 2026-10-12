#include "governance.h"
#include <algorithm>
#include <ctime>

namespace shivrai::governance {

Governance::Governance() = default;

void Governance::SetQuorumBps(uint32_t bps) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (bps <= BPS) quorum_bps_ = bps;
}
void Governance::SetPassThresholdBps(uint32_t bps) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (bps <= BPS) pass_threshold_bps_ = bps;
}
void Governance::SetVotingPeriodSec(int64_t sec) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (sec >= 0) voting_period_sec_ = sec;
}
void Governance::SetTimelockSec(int64_t sec) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (sec >= 0) timelock_sec_ = sec;
}
uint32_t Governance::QuorumBps() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return quorum_bps_;
}
uint32_t Governance::PassThresholdBps() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return pass_threshold_bps_;
}
int64_t Governance::VotingPeriodSec() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return voting_period_sec_;
}
int64_t Governance::TimelockSec() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return timelock_sec_;
}

void Governance::SetVotingPower(const std::string& voter, uint64_t power) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (voter.empty()) return;
    if (power == 0) voting_power_.erase(voter);
    else voting_power_[voter] = power;
}

uint64_t Governance::VotingPowerOf(const std::string& voter) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = voting_power_.find(voter);
    return it == voting_power_.end() ? 0 : it->second;
}

uint64_t Governance::TotalVotingPower() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    uint64_t t = 0;
    for (auto& [k, v] : voting_power_) t += v;
    return t;
}

size_t Governance::VoterCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return voting_power_.size();
}

std::optional<ProposalId> Governance::Propose(const std::string& proposer,
                                              const std::string& title,
                                              const std::string& description) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (proposer.empty() || title.empty()) return std::nullopt;
    if (VotingPowerOf(proposer) == 0) return std::nullopt;

    ProposalId id = next_proposal_id_++;
    Proposal p;
    p.id = id;
    p.proposer = proposer;
    p.title = title;
    p.description = description;
    p.created_at = (int64_t)std::time(nullptr);
    p.quorum_bps = quorum_bps_;
    p.pass_threshold_bps = pass_threshold_bps_;
    p.state = ProposalState::Draft;
    proposals_[id] = p;
    return id;
}

bool Governance::ActivateProposal(ProposalId id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = proposals_.find(id);
    if (it == proposals_.end()) return false;
    if (it->second.state != ProposalState::Draft) return false;
    int64_t now = (int64_t)std::time(nullptr);
    it->second.voting_starts = now;
    it->second.voting_ends = (voting_period_sec_ == 0) ? 0 : (now + voting_period_sec_);
    it->second.snapshot_total_power = TotalVotingPower();
    it->second.state = ProposalState::Active;
    return true;
}

bool Governance::CancelProposal(const std::string& proposer, ProposalId id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = proposals_.find(id);
    if (it == proposals_.end()) return false;
    if (it->second.proposer != proposer) return false;
    if (it->second.state == ProposalState::Executed) return false;
    if (it->second.state == ProposalState::Cancelled) return false;
    if (it->second.state == ProposalState::Queued) return false;
    it->second.state = ProposalState::Cancelled;
    return true;
}

bool Governance::Vote(ProposalId id, const std::string& voter, VoteType type) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = proposals_.find(id);
    if (it == proposals_.end()) return false;
    if (it->second.state != ProposalState::Active) return false;

    int64_t now = (int64_t)std::time(nullptr);
    if (it->second.voting_ends > 0 && now > it->second.voting_ends) return false;

    if (votes_[id].count(voter)) return false;

    uint64_t power = VotingPowerOf(voter);
    if (power == 0) return false;

    VoteRecord rec;
    rec.voter = voter;
    rec.type = type;
    rec.power = power;
    rec.cast_at = now;
    votes_[id][voter] = rec;

    if (type == VoteType::For)          it->second.votes_for += power;
    else if (type == VoteType::Against) it->second.votes_against += power;
    else                                 it->second.votes_abstain += power;
    return true;
}

ProposalResult Governance::Tally(ProposalId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    ProposalResult r;
    auto it = proposals_.find(id);
    if (it == proposals_.end()) return r;

    const Proposal& p = it->second;
    r.total_votes = p.votes_for + p.votes_against + p.votes_abstain;

    if (p.snapshot_total_power > 0) {
        __uint128_t q = (__uint128_t)r.total_votes * BPS / p.snapshot_total_power;
        r.quorum_pct_bps = q > UINT64_MAX ? UINT64_MAX : (uint64_t)q;
        r.quorum_met = r.quorum_pct_bps >= p.quorum_bps;
    }

    uint64_t decisive = p.votes_for + p.votes_against;
    if (decisive > 0) {
        __uint128_t f = (__uint128_t)p.votes_for * BPS / decisive;
        r.for_pct_bps = f > UINT64_MAX ? UINT64_MAX : (uint64_t)f;
    }
    r.passed = r.quorum_met && (r.for_pct_bps > p.pass_threshold_bps);
    return r;
}

bool Governance::FinalizeProposal(ProposalId id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = proposals_.find(id);
    if (it == proposals_.end()) return false;
    if (it->second.state != ProposalState::Active) return false;
    int64_t now = (int64_t)std::time(nullptr);
    if (it->second.voting_ends > 0 && now < it->second.voting_ends) return false;

    ProposalResult r = Tally(id);
    if (r.passed) it->second.state = ProposalState::Succeeded;
    else          it->second.state = ProposalState::Defeated;
    return true;
}

bool Governance::QueueProposal(ProposalId id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = proposals_.find(id);
    if (it == proposals_.end()) return false;
    if (it->second.state != ProposalState::Succeeded) return false;
    int64_t now = (int64_t)std::time(nullptr);
    it->second.queued_at = now;
    it->second.eta = now + timelock_sec_;
    it->second.state = ProposalState::Queued;
    return true;
}

bool Governance::ExecuteProposal(ProposalId id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = proposals_.find(id);
    if (it == proposals_.end()) return false;
    if (it->second.state != ProposalState::Queued) return false;
    int64_t now = (int64_t)std::time(nullptr);
    if (now < it->second.eta) return false;
    it->second.executed_at = now;
    it->second.state = ProposalState::Executed;
    return true;
}

std::optional<Proposal> Governance::GetProposal(ProposalId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = proposals_.find(id);
    if (it == proposals_.end()) return std::nullopt;
    return it->second;
}

std::vector<ProposalId> Governance::AllProposals() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<ProposalId> out;
    for (auto& [k, v] : proposals_) out.push_back(k);
    return out;
}

std::vector<ProposalId> Governance::ProposalsByState(ProposalState s) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<ProposalId> out;
    for (auto& [k, v] : proposals_) if (v.state == s) out.push_back(k);
    return out;
}

std::optional<VoteRecord> Governance::GetVote(ProposalId id, const std::string& voter) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = votes_.find(id);
    if (it == votes_.end()) return std::nullopt;
    auto vit = it->second.find(voter);
    if (vit == it->second.end()) return std::nullopt;
    return vit->second;
}

size_t Governance::ProposalCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return proposals_.size();
}

}