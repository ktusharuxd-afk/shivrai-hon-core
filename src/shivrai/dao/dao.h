#ifndef SHIVRAI_HON_DAO_H
#define SHIVRAI_HON_DAO_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <vector>
namespace shivrai::dao {
using ProposalId = uint64_t;
static constexpr uint32_t BPS = 10000;
enum class PropState { Draft, Voting, Passed, Failed, Executed };
struct DaoProposal {
    ProposalId id{0};
    std::string proposer, title;
    PropState state{PropState::Draft};
    uint64_t for_votes{0}, against_votes{0};
    int64_t created_at{0}, ends_at{0};
    uint64_t quorum{0};
};
class DAO {
public:
    DAO();
    void SetVotingPeriod(int64_t sec);
    void SetQuorum(uint64_t q);
    bool AddMember(const std::string& m, uint64_t power);
    uint64_t MemberPower(const std::string& m) const;
    uint64_t TotalPower() const;
    size_t MemberCount() const;
    std::optional<ProposalId> Propose(const std::string& proposer, const std::string& title);
    bool Vote(ProposalId id, const std::string& voter, bool for_vote);
    bool Tally(ProposalId id);
    bool Execute(ProposalId id);
    std::optional<DaoProposal> Get(ProposalId id) const;
    size_t ProposalCount() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<std::string, uint64_t> members_;
    std::map<ProposalId, DaoProposal> proposals_;
    std::map<ProposalId, std::set<std::string>> voted_;
    ProposalId next_id_{1};
    int64_t voting_period_{3600};
    uint64_t quorum_{0};
};
}
#endif
