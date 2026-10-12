#include "dao.h"
#include <ctime>
namespace shivrai::dao {
DAO::DAO() = default;
void DAO::SetVotingPeriod(int64_t s) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (s >= 0) voting_period_ = s;
}
void DAO::SetQuorum(uint64_t q) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    quorum_ = q;
}
bool DAO::AddMember(const std::string& m, uint64_t p) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (m.empty() || p == 0 || members_.count(m)) return false;
    members_[m] = p;
    return true;
}
uint64_t DAO::MemberPower(const std::string& m) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = members_.find(m);
    return it == members_.end() ? 0 : it->second;
}
uint64_t DAO::TotalPower() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    uint64_t t = 0;
    for (auto& [k, v] : members_) t += v;
    return t;
}
size_t DAO::MemberCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return members_.size();
}
std::optional<ProposalId> DAO::Propose(const std::string& p, const std::string& t) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (p.empty() || t.empty() || !members_.count(p)) return std::nullopt;
    ProposalId id = next_id_++;
    DaoProposal pr;
    pr.id = id; pr.proposer = p; pr.title = t;
    pr.state = PropState::Voting;
    pr.created_at = (int64_t)std::time(nullptr);
    pr.ends_at = pr.created_at + voting_period_;
    pr.quorum = quorum_;
    proposals_[id] = pr;
    return id;
}
bool DAO::Vote(ProposalId id, const std::string& v, bool fv) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = proposals_.find(id);
    if (it == proposals_.end() || it->second.state != PropState::Voting) return false;
    if (voted_[id].count(v)) return false;
    uint64_t p = MemberPower(v);
    if (p == 0) return false;
    if (fv) it->second.for_votes += p;
    else    it->second.against_votes += p;
    voted_[id].insert(v);
    return true;
}
bool DAO::Tally(ProposalId id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = proposals_.find(id);
    if (it == proposals_.end() || it->second.state != PropState::Voting) return false;
    int64_t now = (int64_t)std::time(nullptr);
    if (now < it->second.ends_at) return false;
    uint64_t total = it->second.for_votes + it->second.against_votes;
    if (total < it->second.quorum) { it->second.state = PropState::Failed; return true; }
    if (it->second.for_votes > it->second.against_votes) it->second.state = PropState::Passed;
    else it->second.state = PropState::Failed;
    return true;
}
bool DAO::Execute(ProposalId id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = proposals_.find(id);
    if (it == proposals_.end() || it->second.state != PropState::Passed) return false;
    it->second.state = PropState::Executed;
    return true;
}
std::optional<DaoProposal> DAO::Get(ProposalId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = proposals_.find(id);
    if (it == proposals_.end()) return std::nullopt;
    return it->second;
}
size_t DAO::ProposalCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return proposals_.size();
}
}
