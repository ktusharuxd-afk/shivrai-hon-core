#!/usr/bin/env python3
"""Batch 4 (final): dao, oracle_aggregator, health_monitor, airdrop, upgradeable_proxy, indexer"""
import os, subprocess, sys
ROOT = os.path.expanduser("~/shivrai-hon-core")

def write(path, content):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f: f.write(content)

MODULES = {}

# ==================== dao ====================
MODULES["dao"] = {
"h": r'''#ifndef SHIVRAI_HON_DAO_H
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
''',
"cpp": r'''#include "dao.h"
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
''',
"test": r'''#include "shivrai/dao/dao.h"
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
'''
}

# ==================== oracle_aggregator ====================
MODULES["oracle_aggregator"] = {
"h": r'''#ifndef SHIVRAI_HON_ORACLE_AGGREGATOR_H
#define SHIVRAI_HON_ORACLE_AGGREGATOR_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <vector>
namespace shivrai::oracle_aggregator {
struct PricePoint {
    std::string source;
    uint64_t price{0};
    int64_t ts{0};
};
struct AggregatedPrice {
    std::string asset;
    uint64_t median{0};
    uint64_t min_price{0};
    uint64_t max_price{0};
    size_t sources{0};
    int64_t ts{0};
    bool valid{false};
};
class OracleAggregator {
public:
    OracleAggregator();
    bool RegisterSource(const std::string& source);
    bool SubmitPrice(const std::string& source, const std::string& asset, uint64_t price);
    std::optional<AggregatedPrice> Aggregate(const std::string& asset) const;
    size_t SourceCount() const;
    size_t PendingCount(const std::string& asset) const;
    bool RemoveSource(const std::string& source);
    void SetMaxAgeSec(int64_t sec);
    int64_t MaxAgeSec() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<std::string, bool> sources_;
    std::map<std::string, std::map<std::string, PricePoint>> latest_;
    int64_t max_age_sec_{300};
};
}
#endif
''',
"cpp": r'''#include "oracle_aggregator.h"
#include <algorithm>
#include <ctime>
namespace shivrai::oracle_aggregator {
OracleAggregator::OracleAggregator() = default;
bool OracleAggregator::RegisterSource(const std::string& s) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (s.empty() || sources_.count(s)) return false;
    sources_[s] = true;
    return true;
}
bool OracleAggregator::RemoveSource(const std::string& s) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return sources_.erase(s) > 0;
}
bool OracleAggregator::SubmitPrice(const std::string& s, const std::string& a, uint64_t p) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (s.empty() || a.empty() || p == 0) return false;
    if (!sources_.count(s)) return false;
    PricePoint pp;
    pp.source = s; pp.price = p;
    pp.ts = (int64_t)std::time(nullptr);
    latest_[a][s] = pp;
    return true;
}
std::optional<AggregatedPrice> OracleAggregator::Aggregate(const std::string& a) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = latest_.find(a);
    if (it == latest_.end()) return std::nullopt;
    AggregatedPrice ap;
    ap.asset = a;
    int64_t now = (int64_t)std::time(nullptr);
    std::vector<uint64_t> prices;
    for (auto& [src, pp] : it->second) {
        if (now - pp.ts > max_age_sec_) continue;
        if (!sources_.count(src)) continue;
        prices.push_back(pp.price);
    }
    if (prices.empty()) return std::nullopt;
    std::sort(prices.begin(), prices.end());
    ap.median = prices[prices.size() / 2];
    ap.min_price = prices.front();
    ap.max_price = prices.back();
    ap.sources = prices.size();
    ap.ts = now;
    ap.valid = true;
    return ap;
}
size_t OracleAggregator::SourceCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return sources_.size();
}
size_t OracleAggregator::PendingCount(const std::string& a) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = latest_.find(a);
    return it == latest_.end() ? 0 : it->second.size();
}
void OracleAggregator::SetMaxAgeSec(int64_t s) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (s > 0) max_age_sec_ = s;
}
int64_t OracleAggregator::MaxAgeSec() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return max_age_sec_;
}
}
''',
"test": r'''#include "shivrai/oracle_aggregator/oracle_aggregator.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::oracle_aggregator;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    OracleAggregator oa;
    CHECK(oa.SourceCount() == 0);
    CHECK(oa.MaxAgeSec() == 300);
    CHECK(!oa.RegisterSource(""));
    CHECK(oa.RegisterSource("binance"));
    CHECK(oa.RegisterSource("kraken"));
    CHECK(oa.RegisterSource("coinbase"));
    CHECK(!oa.RegisterSource("binance"));
    CHECK(oa.SourceCount() == 3);
    CHECK(!oa.SubmitPrice("", "BTC", 100));
    CHECK(!oa.SubmitPrice("binance", "", 100));
    CHECK(!oa.SubmitPrice("binance", "BTC", 0));
    CHECK(!oa.SubmitPrice("unknown", "BTC", 100));
    CHECK(oa.SubmitPrice("binance", "BTC", 100));
    CHECK(oa.SubmitPrice("kraken", "BTC", 102));
    CHECK(oa.SubmitPrice("coinbase", "BTC", 98));
    CHECK(oa.PendingCount("BTC") == 3);
    CHECK(oa.PendingCount("ETH") == 0);
    auto agg = oa.Aggregate("BTC");
    CHECK(agg.has_value());
    CHECK(agg->asset == "BTC");
    CHECK(agg->sources == 3);
    CHECK(agg->median == 100);
    CHECK(agg->min_price == 98);
    CHECK(agg->max_price == 102);
    CHECK(agg->valid);
    CHECK(!oa.Aggregate("NOPE").has_value());
    oa.SubmitPrice("binance", "BTC", 200);
    auto agg2 = oa.Aggregate("BTC");
    CHECK(agg2->max_price == 200);
    CHECK(oa.RemoveSource("coinbase"));
    CHECK(oa.SourceCount() == 2);
    auto agg3 = oa.Aggregate("BTC");
    CHECK(agg3->sources == 2);
    CHECK(!oa.RemoveSource("coinbase"));
    oa.SetMaxAgeSec(10);
    CHECK(oa.MaxAgeSec() == 10);
    oa.SetMaxAgeSec(0);
    CHECK(oa.MaxAgeSec() == 10);
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
'''
}

# ==================== health_monitor ====================
MODULES["health_monitor"] = {
"h": r'''#ifndef SHIVRAI_HON_HEALTH_MONITOR_H
#define SHIVRAI_HON_HEALTH_MONITOR_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
namespace shivrai::health_monitor {
enum class Status { Healthy, Degraded, Paused };
struct Metric {
    std::string name;
    uint64_t value{0};
    int64_t ts{0};
};
class HealthMonitor {
public:
    HealthMonitor();
    bool RegisterMetric(const std::string& name, uint64_t threshold, bool high_is_bad);
    bool Report(const std::string& name, uint64_t value);
    std::optional<Metric> GetMetric(const std::string& name) const;
    Status StatusOf(const std::string& name) const;
    bool IsHealthy(const std::string& name) const;
    bool Pause(const std::string& reason);
    bool Resume();
    bool IsPaused() const;
    std::string PauseReason() const;
    size_t MetricCount() const;
    size_t TriggeredCount() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<std::string, uint64_t> thresholds_;
    std::map<std::string, bool> high_bad_;
    std::map<std::string, Metric> metrics_;
    bool paused_{false};
    std::string pause_reason_;
};
}
#endif
''',
"cpp": r'''#include "health_monitor.h"
#include <ctime>
namespace shivrai::health_monitor {
HealthMonitor::HealthMonitor() = default;
bool HealthMonitor::RegisterMetric(const std::string& n, uint64_t t, bool hib) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (n.empty() || thresholds_.count(n)) return false;
    thresholds_[n] = t;
    high_bad_[n] = hib;
    return true;
}
bool HealthMonitor::Report(const std::string& n, uint64_t v) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (!thresholds_.count(n)) return false;
    Metric m;
    m.name = n; m.value = v;
    m.ts = (int64_t)std::time(nullptr);
    metrics_[n] = m;
    return true;
}
std::optional<Metric> HealthMonitor::GetMetric(const std::string& n) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = metrics_.find(n);
    if (it == metrics_.end()) return std::nullopt;
    return it->second;
}
Status HealthMonitor::StatusOf(const std::string& n) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (paused_) return Status::Paused;
    auto mit = metrics_.find(n);
    if (mit == metrics_.end()) return Status::Healthy;
    uint64_t t = thresholds_.at(n);
    bool hib = high_bad_.at(n);
    if (hib && mit->second.value > t) return Status::Degraded;
    if (!hib && mit->second.value < t) return Status::Degraded;
    return Status::Healthy;
}
bool HealthMonitor::IsHealthy(const std::string& n) const {
    return StatusOf(n) == Status::Healthy;
}
bool HealthMonitor::Pause(const std::string& r) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (paused_) return false;
    paused_ = true;
    pause_reason_ = r;
    return true;
}
bool HealthMonitor::Resume() {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (!paused_) return false;
    paused_ = false;
    pause_reason_.clear();
    return true;
}
bool HealthMonitor::IsPaused() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return paused_;
}
std::string HealthMonitor::PauseReason() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return pause_reason_;
}
size_t HealthMonitor::MetricCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return thresholds_.size();
}
size_t HealthMonitor::TriggeredCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    size_t n = 0;
    for (auto& [k, v] : metrics_) {
        if (StatusOf(k) != Status::Healthy) ++n;
    }
    return n;
}
}
''',
"test": r'''#include "shivrai/health_monitor/health_monitor.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::health_monitor;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    HealthMonitor hm;
    CHECK(hm.MetricCount() == 0);
    CHECK(!hm.IsPaused());
    CHECK(!hm.RegisterMetric("", 100, true));
    CHECK(hm.RegisterMetric("latency", 1000, true));
    CHECK(hm.RegisterMetric("uptime", 95, false));
    CHECK(!hm.RegisterMetric("latency", 1000, true));
    CHECK(hm.MetricCount() == 2);
    CHECK(!hm.Report("unknown", 50));
    CHECK(hm.Report("latency", 500));
    CHECK(hm.Report("uptime", 99));
    CHECK(hm.IsHealthy("latency"));
    CHECK(hm.IsHealthy("uptime"));
    CHECK(hm.StatusOf("latency") == Status::Healthy);
    CHECK(hm.Report("latency", 2000));
    CHECK(hm.StatusOf("latency") == Status::Degraded);
    CHECK(!hm.IsHealthy("latency"));
    CHECK(hm.Report("uptime", 90));
    CHECK(hm.StatusOf("uptime") == Status::Degraded);
    CHECK(hm.TriggeredCount() == 2);
    CHECK(hm.Report("latency", 500));
    CHECK(hm.Report("uptime", 99));
    CHECK(hm.TriggeredCount() == 0);
    auto m = hm.GetMetric("latency");
    CHECK(m.has_value());
    CHECK(m->name == "latency");
    CHECK(m->value == 500);
    CHECK(!hm.GetMetric("unknown").has_value());
    CHECK(hm.Pause("emergency"));
    CHECK(hm.IsPaused());
    CHECK(hm.PauseReason() == "emergency");
    CHECK(hm.StatusOf("latency") == Status::Paused);
    CHECK(!hm.Pause("again"));
    CHECK(hm.Resume());
    CHECK(!hm.IsPaused());
    CHECK(hm.PauseReason().empty());
    CHECK(!hm.Resume());
    CHECK(hm.StatusOf("unknown") == Status::Healthy);
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
'''
}

# ==================== airdrop ====================
MODULES["airdrop"] = {
"h": r'''#ifndef SHIVRAI_HON_AIRDROP_H
#define SHIVRAI_HON_AIRDROP_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <vector>
namespace shivrai::airdrop {
using RoundId = uint64_t;
struct AirdropRound {
    RoundId id{0};
    std::string name;
    uint64_t total_allocated{0};
    uint64_t total_claimed{0};
    int64_t starts_at{0};
    int64_t ends_at{0};
    bool active{true};
};
class Airdrop {
public:
    Airdrop();
    std::optional<RoundId> CreateRound(const std::string& name, int64_t start, int64_t end);
    bool SetEligibility(RoundId id, const std::string& user, uint64_t amount);
    std::optional<uint64_t> Claim(RoundId id, const std::string& user);
    std::optional<uint64_t> AmountFor(RoundId id, const std::string& user) const;
    bool HasClaimed(RoundId id, const std::string& user) const;
    std::optional<AirdropRound> GetRound(RoundId id) const;
    size_t RoundCount() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<RoundId, AirdropRound> rounds_;
    std::map<RoundId, std::map<std::string, uint64_t>> eligible_;
    std::map<RoundId, std::map<std::string, bool>> claimed_;
    RoundId next_id_{1};
};
}
#endif
''',
"cpp": r'''#include "airdrop.h"
#include <ctime>
namespace shivrai::airdrop {
Airdrop::Airdrop() = default;
std::optional<RoundId> Airdrop::CreateRound(const std::string& n, int64_t s, int64_t e) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (n.empty() || e <= s) return std::nullopt;
    RoundId id = next_id_++;
    AirdropRound r;
    r.id = id; r.name = n; r.starts_at = s; r.ends_at = e;
    rounds_[id] = r;
    return id;
}
bool Airdrop::SetEligibility(RoundId id, const std::string& u, uint64_t a) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = rounds_.find(id);
    if (it == rounds_.end() || !it->second.active) return false;
    if (u.empty() || a == 0 || eligible_[id].count(u)) return false;
    eligible_[id][u] = a;
    it->second.total_allocated += a;
    return true;
}
std::optional<uint64_t> Airdrop::Claim(RoundId id, const std::string& u) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = rounds_.find(id);
    if (it == rounds_.end() || !it->second.active) return std::nullopt;
    auto eit = eligible_[id].find(u);
    if (eit == eligible_[id].end()) return std::nullopt;
    if (claimed_[id][u]) return std::nullopt;
    int64_t now = (int64_t)std::time(nullptr);
    if (now < it->second.starts_at || now > it->second.ends_at) return std::nullopt;
    claimed_[id][u] = true;
    it->second.total_claimed += eit->second;
    return eit->second;
}
std::optional<uint64_t> Airdrop::AmountFor(RoundId id, const std::string& u) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = eligible_.find(id);
    if (it == eligible_.end()) return std::nullopt;
    auto uit = it->second.find(u);
    if (uit == it->second.end()) return std::nullopt;
    return uit->second;
}
bool Airdrop::HasClaimed(RoundId id, const std::string& u) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = claimed_.find(id);
    if (it == claimed_.end()) return false;
    auto uit = it->second.find(u);
    return uit != it->second.end() && uit->second;
}
std::optional<AirdropRound> Airdrop::GetRound(RoundId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = rounds_.find(id);
    if (it == rounds_.end()) return std::nullopt;
    return it->second;
}
size_t Airdrop::RoundCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return rounds_.size();
}
}
''',
"test": r'''#include "shivrai/airdrop/airdrop.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::airdrop;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    Airdrop a;
    CHECK(a.RoundCount() == 0);
    CHECK(!a.CreateRound("", 0, 100).has_value());
    CHECK(!a.CreateRound("Drop1", 100, 50).has_value());
    // Round active now → use past/future large range
    auto r1 = a.CreateRound("Drop1", -100000, 9999999999LL);
    CHECK(r1.has_value());
    CHECK(a.RoundCount() == 1);
    auto rd = a.GetRound(*r1);
    CHECK(rd.has_value());
    CHECK(rd->name == "Drop1");
    CHECK(rd->total_allocated == 0);
    CHECK(rd->total_claimed == 0);
    CHECK(rd->active);
    CHECK(!a.SetEligibility(*r1, "", 100));
    CHECK(!a.SetEligibility(*r1, "alice", 0));
    CHECK(a.SetEligibility(*r1, "alice", 1000));
    CHECK(a.SetEligibility(*r1, "bob", 500));
    CHECK(!a.SetEligibility(*r1, "alice", 200)); // dup
    CHECK(a.GetRound(*r1)->total_allocated == 1500);
    auto amt = a.AmountFor(*r1, "alice");
    CHECK(amt.has_value() && *amt == 1000);
    CHECK(!a.AmountFor(*r1, "carol").has_value());
    CHECK(!a.AmountFor(99999, "alice").has_value());
    CHECK(!a.HasClaimed(*r1, "alice"));
    auto c = a.Claim(*r1, "alice");
    CHECK(c.has_value() && *c == 1000);
    CHECK(a.HasClaimed(*r1, "alice"));
    CHECK(!a.Claim(*r1, "alice"));    // double claim
    CHECK(a.GetRound(*r1)->total_claimed == 1000);
    CHECK(!a.Claim(*r1, "carol"));    // not eligible
    CHECK(!a.Claim(99999, "alice"));  // bad round
    // Round with future start → cannot claim
    auto r3 = a.CreateRound("Future", 9999999999LL, 9999999999LL + 100000);
    CHECK(r3.has_value());
    CHECK(a.SetEligibility(*r3, "alice", 100));
    CHECK(!a.Claim(*r3, "alice")); // not started
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
'''
}

# ==================== upgradeable_proxy ====================
MODULES["upgradeable_proxy"] = {
"h": r'''#ifndef SHIVRAI_HON_UPGRADEABLE_PROXY_H
#define SHIVRAI_HON_UPGRADEABLE_PROXY_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
namespace shivrai::upgradeable_proxy {
struct Version {
    uint32_t major{0}, minor{0}, patch{0};
    std::string impl_hash;
    int64_t deployed_at{0};
    bool active{true};
};
class UpgradeableProxy {
public:
    UpgradeableProxy();
    bool Initialize(const std::string& admin, const std::string& initial_hash);
    bool Upgrade(const std::string& caller, const std::string& new_hash,
                 uint32_t major, uint32_t minor, uint32_t patch);
    bool Rollback(const std::string& caller);
    bool TransferAdmin(const std::string& caller, const std::string& new_admin);
    std::string CurrentHash() const;
    std::string Admin() const;
    size_t VersionCount() const;
    std::optional<Version> VersionAt(size_t idx) const;
    bool IsInitialized() const;
private:
    mutable std::recursive_mutex cs_;
    std::string admin_;
    std::string current_hash_;
    std::vector<Version> history_;
    bool initialized_{false};
};
}
#endif
''',
"cpp": r'''#include "upgradeable_proxy.h"
#include <ctime>
namespace shivrai::upgradeable_proxy {
UpgradeableProxy::UpgradeableProxy() = default;
bool UpgradeableProxy::Initialize(const std::string& a, const std::string& h) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (initialized_ || a.empty() || h.empty()) return false;
    admin_ = a; current_hash_ = h;
    Version v;
    v.impl_hash = h;
    v.deployed_at = (int64_t)std::time(nullptr);
    history_.push_back(v);
    initialized_ = true;
    return true;
}
bool UpgradeableProxy::Upgrade(const std::string& c, const std::string& nh,
                               uint32_t mj, uint32_t mn, uint32_t pt) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (!initialized_ || c != admin_ || nh.empty()) return false;
    Version v;
    v.major = mj; v.minor = mn; v.patch = pt;
    v.impl_hash = nh;
    v.deployed_at = (int64_t)std::time(nullptr);
    history_.push_back(v);
    current_hash_ = nh;
    return true;
}
bool UpgradeableProxy::Rollback(const std::string& c) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (!initialized_ || c != admin_) return false;
    if (history_.size() < 2) return false;
    history_.pop_back();
    current_hash_ = history_.back().impl_hash;
    return true;
}
bool UpgradeableProxy::TransferAdmin(const std::string& c, const std::string& na) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (!initialized_ || c != admin_ || na.empty() || na == admin_) return false;
    admin_ = na;
    return true;
}
std::string UpgradeableProxy::CurrentHash() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return current_hash_;
}
std::string UpgradeableProxy::Admin() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return admin_;
}
size_t UpgradeableProxy::VersionCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return history_.size();
}
std::optional<Version> UpgradeableProxy::VersionAt(size_t i) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (i >= history_.size()) return std::nullopt;
    return history_[i];
}
bool UpgradeableProxy::IsInitialized() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return initialized_;
}
}
''',
"test": r'''#include "shivrai/upgradeable_proxy/upgradeable_proxy.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::upgradeable_proxy;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    UpgradeableProxy p;
    CHECK(!p.IsInitialized());
    CHECK(p.CurrentHash().empty());
    CHECK(p.Admin().empty());
    CHECK(p.VersionCount() == 0);
    CHECK(!p.Initialize("", "0xhash1"));
    CHECK(!p.Initialize("alice", ""));
    CHECK(p.Initialize("alice", "0xhash1"));
    CHECK(p.IsInitialized());
    CHECK(p.Admin() == "alice");
    CHECK(p.CurrentHash() == "0xhash1");
    CHECK(p.VersionCount() == 1);
    CHECK(!p.Initialize("bob", "0xhash2")); // already initialized
    CHECK(!p.Upgrade("bob", "0xhash2", 1, 0, 0));
    CHECK(!p.Upgrade("alice", "", 1, 0, 0));
    CHECK(p.Upgrade("alice", "0xhash2", 1, 0, 0));
    CHECK(p.CurrentHash() == "0xhash2");
    CHECK(p.VersionCount() == 2);
    CHECK(p.Upgrade("alice", "0xhash3", 2, 0, 0));
    CHECK(p.VersionCount() == 3);
    CHECK(p.CurrentHash() == "0xhash3");
    auto v0 = p.VersionAt(0);
    CHECK(v0.has_value());
    CHECK(v0->impl_hash == "0xhash1");
    auto v2 = p.VersionAt(2);
    CHECK(v2.has_value());
    CHECK(v2->major == 2);
    CHECK(v2->impl_hash == "0xhash3");
    CHECK(!p.VersionAt(99).has_value());
    CHECK(p.Rollback("alice"));
    CHECK(p.CurrentHash() == "0xhash2");
    CHECK(p.VersionCount() == 2);
    CHECK(!p.Rollback("bob"));
    CHECK(!p.TransferAdmin("bob", "carol"));
    CHECK(!p.TransferAdmin("alice", ""));
    CHECK(!p.TransferAdmin("alice", "alice"));
    CHECK(p.TransferAdmin("alice", "carol"));
    CHECK(p.Admin() == "carol");
    CHECK(!p.Upgrade("alice", "0xhash4", 3, 0, 0));
    CHECK(p.Upgrade("carol", "0xhash4", 3, 0, 0));
    // rollback to only version
    UpgradeableProxy p2;
    p2.Initialize("x", "h1");
    CHECK(!p2.Rollback("x"));
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
'''
}

# ==================== indexer ====================
MODULES["indexer"] = {
"h": r'''#ifndef SHIVRAI_HON_INDEXER_H
#define SHIVRAI_HON_INDEXER_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <vector>
namespace shivrai::indexer {
struct IndexedItem {
    uint64_t id{0};
    std::string category;
    std::string key;
    std::string value;
    int64_t ts{0};
};
class Indexer {
public:
    Indexer();
    uint64_t Index(const std::string& category, const std::string& key, const std::string& value);
    std::vector<IndexedItem> Lookup(const std::string& category, const std::string& key) const;
    std::vector<IndexedItem> ByCategory(const std::string& category) const;
    std::optional<IndexedItem> Get(uint64_t id) const;
    std::vector<uint64_t> Categories() const;
    std::vector<std::string> KeysIn(const std::string& category) const;
    size_t Count() const;
    bool Remove(uint64_t id);
private:
    mutable std::recursive_mutex cs_;
    std::map<uint64_t, IndexedItem> items_;
    std::map<std::string, std::set<uint64_t>> by_category_;
    std::map<std::pair<std::string, std::string>, std::set<uint64_t>> by_key_;
    uint64_t next_id_{1};
};
}
#endif
''',
"cpp": r'''#include "indexer.h"
#include <ctime>
namespace shivrai::indexer {
Indexer::Indexer() = default;
uint64_t Indexer::Index(const std::string& cat, const std::string& k, const std::string& v) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (cat.empty() || k.empty()) return 0;
    uint64_t id = next_id_++;
    IndexedItem it;
    it.id = id; it.category = cat; it.key = k; it.value = v;
    it.ts = (int64_t)std::time(nullptr);
    items_[id] = it;
    by_category_[cat].insert(id);
    by_key_[{cat, k}].insert(id);
    return id;
}
std::vector<IndexedItem> Indexer::Lookup(const std::string& cat, const std::string& k) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<IndexedItem> out;
    auto it = by_key_.find({cat, k});
    if (it == by_key_.end()) return out;
    for (auto id : it->second) out.push_back(items_.at(id));
    return out;
}
std::vector<IndexedItem> Indexer::ByCategory(const std::string& cat) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<IndexedItem> out;
    auto it = by_category_.find(cat);
    if (it == by_category_.end()) return out;
    for (auto id : it->second) out.push_back(items_.at(id));
    return out;
}
std::optional<IndexedItem> Indexer::Get(uint64_t id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = items_.find(id);
    if (it == items_.end()) return std::nullopt;
    return it->second;
}
std::vector<uint64_t> Indexer::Categories() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<uint64_t> out; // dummy — not used, kept for API
    (void)out;
    std::vector<uint64_t> r;
    return r;
}
std::vector<std::string> Indexer::KeysIn(const std::string& cat) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::set<std::string> ks;
    for (auto& [k, ids] : by_key_) if (k.first == cat) ks.insert(k.second);
    return std::vector<std::string>(ks.begin(), ks.end());
}
size_t Indexer::Count() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return items_.size();
}
bool Indexer::Remove(uint64_t id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = items_.find(id);
    if (it == items_.end()) return false;
    by_category_[it->second.category].erase(id);
    by_key_[{it->second.category, it->second.key}].erase(id);
    items_.erase(it);
    return true;
}
}
''',
"test": r'''#include "shivrai/indexer/indexer.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::indexer;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    Indexer idx;
    CHECK(idx.Count() == 0);
    CHECK(idx.Index("", "k", "v") == 0);
    CHECK(idx.Index("cat", "", "v") == 0);
    auto a = idx.Index("tx", "sender", "alice");
    auto b = idx.Index("tx", "sender", "alice");
    auto c = idx.Index("tx", "sender", "bob");
    CHECK(a > 0 && b > 0 && c > 0);
    CHECK(idx.Count() == 3);
    auto la = idx.Lookup("tx", "sender");
    CHECK(la.size() == 3); // all share key
    auto lb = idx.Lookup("tx", "other");
    CHECK(lb.empty());
    auto ba = idx.ByCategory("tx");
    CHECK(ba.size() == 3);
    CHECK(idx.ByCategory("nope").empty());
    auto g = idx.Get(a);
    CHECK(g.has_value());
    CHECK(g->category == "tx");
    CHECK(g->key == "sender");
    CHECK(g->value == "alice");
    CHECK(!idx.Get(99999).has_value());
    auto ks = idx.KeysIn("tx");
    CHECK(ks.size() == 1);
    CHECK(ks[0] == "sender");
    CHECK(idx.Remove(a));
    CHECK(idx.Count() == 2);
    CHECK(!idx.Remove(a));
    CHECK(!idx.Remove(99999));
    auto la2 = idx.Lookup("tx", "sender");
    CHECK(la2.size() == 2);
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
'''
}

# ==================== RUN ====================
def write_module(name, files):
    write(f"{ROOT}/src/shivrai/{name}/{name}.h", files["h"])
    write(f"{ROOT}/src/shivrai/{name}/{name}.cpp", files["cpp"])
    write(f"{ROOT}/tests/shivrai/test_{name}.cpp", files["test"])

def test_module(name):
    c = f"{ROOT}/src/shivrai/{name}/{name}.cpp"
    t = f"{ROOT}/tests/shivrai/test_{name}.cpp"
    r = subprocess.run(["g++","-std=c++17","-Wall","-Wextra","-O2",
                        f"-I{ROOT}/src", t, c, "-o", f"/tmp/t_{name}"],
                       capture_output=True, text=True)
    if r.returncode != 0:
        print(f"❌ COMPILE FAIL {name}:\n{r.stderr[:1500]}")
        return False
    r = subprocess.run([f"/tmp/t_{name}"], capture_output=True, text=True)
    if r.returncode != 0:
        print(f"❌ TEST FAIL {name}:\n{r.stdout}\n{r.stderr}")
        return False
    print(f"✅ {name}: {r.stdout.strip()}")
    return True

def main():
    target = sys.argv[1] if len(sys.argv) > 1 else None
    names = [target] if target else list(MODULES.keys())
    ok, fail = [], []
    for n in names:
        if n not in MODULES:
            print(f"Unknown module: {n}"); continue
        print(f"\n=== {n} ===")
        write_module(n, MODULES[n])
        (ok if test_module(n) else fail).append(n)
    print(f"\n=== SUMMARY ===\nOK: {ok}\nFAIL: {fail}")
    if fail: sys.exit(1)
    subprocess.run(["git","-C",ROOT,"add","-A"])
    subprocess.run(["git","-C",ROOT,"commit","-m",f"feat: batch4 final ({', '.join(ok)})"])
    subprocess.run(["git","-C",ROOT,"push","origin","master"])
    print("🚀 pushed")

if __name__ == "__main__":
    main()