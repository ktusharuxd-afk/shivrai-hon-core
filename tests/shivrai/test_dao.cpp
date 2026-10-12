#include "shivrai/dao/dao.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::dao;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    DAO d;
    d.SetVotingPeriod(0);
    d.SetQuorum(500);
    CHECK(d.MemberCount() == 0);
    CHECK(d.TotalPower() == 0);
    CHECK(!d.AddMember("", 100));
    CHECK(!d.AddMember("alice", 0));
    CHECK(d.AddMember("alice", 1000));
    CHECK(d.AddMember("bob", 800));
    CHECK(d.AddMember("carol", 400));
    CHECK(!d.AddMember("alice", 500));
    CHECK(d.MemberCount() == 3);
    CHECK(d.MemberPower("alice") == 1000);
    CHECK(d.MemberPower("nope") == 0);
    CHECK(d.TotalPower() == 2200);
    CHECK(!d.Propose("", "T").has_value());
    CHECK(!d.Propose("alice", "").has_value());
    CHECK(!d.Propose("nope", "T").has_value());
    auto p1 = d.Propose("alice", "Fund dev");
    CHECK(p1.has_value());
    CHECK(d.ProposalCount() == 1);
    CHECK(d.Get(*p1)->state == PropState::Voting);
    CHECK(d.Vote(*p1, "alice", true));
    CHECK(d.Vote(*p1, "bob", true));
    CHECK(d.Vote(*p1, "carol", false));
    CHECK(!d.Vote(*p1, "alice", true));      // double
    CHECK(!d.Vote(*p1, "nope", true));       // not member
    CHECK(!d.Vote(99999, "alice", true));
    CHECK(d.Get(*p1)->for_votes == 1800);
    CHECK(d.Get(*p1)->against_votes == 400);
    CHECK(d.Tally(*p1));
    CHECK(d.Get(*p1)->state == PropState::Passed);
    CHECK(d.Execute(*p1));
    CHECK(d.Get(*p1)->state == PropState::Executed);
    CHECK(!d.Execute(*p1));
    CHECK(!d.Get(99999).has_value());
    // Proposal that fails quorum
    d.SetQuorum(5000);
    auto p2 = d.Propose("alice", "High quorum");
    CHECK(d.Vote(*p2, "carol", true));
    CHECK(d.Tally(*p2));
    CHECK(d.Get(*p2)->state == PropState::Failed);
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
