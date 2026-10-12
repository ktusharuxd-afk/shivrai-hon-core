#!/usr/bin/env python3
"""Batch 3: randomness_beacon, wrapped_tokens, kyc_registry, ticket_system, subscription"""
import os, subprocess, sys
ROOT = os.path.expanduser("~/shivrai-hon-core")

def write(path, content):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f: f.write(content)

MODULES = {}

# ==================== randomness_beacon ====================
MODULES["randomness_beacon"] = {
"h": r'''#ifndef SHIVRAI_HON_RANDOMNESS_BEACON_H
#define SHIVRAI_HON_RANDOMNESS_BEACON_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>
namespace shivrai::randomness_beacon {
using RoundId = uint64_t;
struct Round {
    RoundId id{0};
    std::string seed;
    std::string result;
    int64_t opened_at{0};
    int64_t revealed_at{0};
    std::set<std::string> commitments;
    std::map<std::string, std::string> reveals;
    bool finalized{false};
};
class Beacon {
public:
    Beacon();
    std::optional<RoundId> OpenRound(const std::string& seed);
    bool Commit(RoundId id, const std::string& participant, const std::string& commitment);
    bool Reveal(RoundId id, const std::string& participant, const std::string& value);
    bool Finalize(RoundId id);
    std::optional<Round> GetRound(RoundId id) const;
    std::string Combine(RoundId id) const;
    size_t RoundCount() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<RoundId, Round> rounds_;
    RoundId next_id_{1};
};
}
#endif
''',
"cpp": r'''#include "randomness_beacon.h"
#include <ctime>
namespace shivrai::randomness_beacon {
Beacon::Beacon() = default;
std::optional<RoundId> Beacon::OpenRound(const std::string& seed) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (seed.empty()) return std::nullopt;
    RoundId id = next_id_++;
    Round r;
    r.id = id; r.seed = seed;
    r.opened_at = (int64_t)std::time(nullptr);
    rounds_[id] = r;
    return id;
}
bool Beacon::Commit(RoundId id, const std::string& p, const std::string& c) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = rounds_.find(id);
    if (it == rounds_.end() || it->second.finalized) return false;
    if (p.empty() || c.empty()) return false;
    if (it->second.commitments.count(p)) return false;
    it->second.commitments.insert(p);
    return true;
}
bool Beacon::Reveal(RoundId id, const std::string& p, const std::string& v) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = rounds_.find(id);
    if (it == rounds_.end() || it->second.finalized) return false;
    if (!it->second.commitments.count(p)) return false;
    if (it->second.reveals.count(p)) return false;
    it->second.reveals[p] = v;
    return true;
}
bool Beacon::Finalize(RoundId id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = rounds_.find(id);
    if (it == rounds_.end() || it->second.finalized) return false;
    if (it->second.reveals.empty()) return false;
    it->second.result = Combine(id);
    it->second.revealed_at = (int64_t)std::time(nullptr);
    it->second.finalized = true;
    return true;
}
std::optional<Round> Beacon::GetRound(RoundId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = rounds_.find(id);
    if (it == rounds_.end()) return std::nullopt;
    return it->second;
}
std::string Beacon::Combine(RoundId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = rounds_.find(id);
    if (it == rounds_.end()) return "";
    std::string combined = it->second.seed;
    for (auto& [p, v] : it->second.reveals) combined += "|" + p + ":" + v;
    uint64_t h = 1469598103934665603ULL;
    for (char c : combined) { h ^= (uint8_t)c; h *= 1099511628211ULL; }
    char buf[32];
    snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)h);
    return std::string(buf);
}
size_t Beacon::RoundCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return rounds_.size();
}
}
''',
"test": r'''#include "shivrai/randomness_beacon/randomness_beacon.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::randomness_beacon;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    Beacon b;
    CHECK(b.RoundCount() == 0);
    CHECK(!b.OpenRound("").has_value());
    auto r1 = b.OpenRound("seed1");
    CHECK(r1.has_value());
    CHECK(b.RoundCount() == 1);
    CHECK(b.GetRound(*r1)->seed == "seed1");
    CHECK(b.GetRound(*r1)->finalized == false);
    CHECK(!b.Commit(*r1, "", "c"));
    CHECK(!b.Commit(*r1, "alice", ""));
    CHECK(b.Commit(*r1, "alice", "H1"));
    CHECK(!b.Commit(*r1, "alice", "H2"));
    CHECK(b.Commit(*r1, "bob", "H2"));
    CHECK(!b.Reveal(*r1, "carol", "v"));  // no commitment
    CHECK(b.Reveal(*r1, "alice", "s1"));
    CHECK(!b.Reveal(*r1, "alice", "s1b")); // dup
    CHECK(b.Reveal(*r1, "bob", "s2"));
    CHECK(b.Finalize(*r1));
    CHECK(b.GetRound(*r1)->finalized);
    CHECK(!b.Commit(*r1, "carol", "H3"));
    CHECK(!b.Reveal(*r1, "carol", "s3"));
    CHECK(!b.Finalize(*r1));
    auto result = b.GetRound(*r1)->result;
    CHECK(!result.empty());
    CHECK(b.Combine(*r1) == result);
    auto r2 = b.OpenRound("seed2");
    CHECK(!b.Finalize(*r2)); // no reveals
    CHECK(!b.GetRound(99999).has_value());
    CHECK(b.Combine(99999) == "");
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
'''
}

# ==================== wrapped_tokens ====================
MODULES["wrapped_tokens"] = {
"h": r'''#ifndef SHIVRAI_HON_WRAPPED_TOKENS_H
#define SHIVRAI_HON_WRAPPED_TOKENS_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
namespace shivrai::wrapped_tokens {
struct WrappedAsset {
    std::string symbol;
    std::string underlying_chain;
    std::string underlying_symbol;
    uint64_t    total_supply{0};
    uint64_t    total_backing{0};
    bool        active{true};
};
class WrappedTokens {
public:
    WrappedTokens();
    bool Register(const std::string& symbol, const std::string& chain, const std::string& underlying);
    bool SetActive(const std::string& symbol, bool active);
    std::optional<WrappedAsset> Get(const std::string& symbol) const;
    bool Wrap(const std::string& symbol, const std::string& user, uint64_t amount);
    std::optional<uint64_t> Unwrap(const std::string& symbol, const std::string& user, uint64_t amount);
    uint64_t BalanceOf(const std::string& symbol, const std::string& user) const;
    uint64_t TotalSupply(const std::string& symbol) const;
    uint64_t TotalBacking(const std::string& symbol) const;
    bool IsFullyBacked(const std::string& symbol) const;
    size_t AssetCount() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<std::string, WrappedAsset> assets_;
    std::map<std::string, std::map<std::string, uint64_t>> balances_;
};
}
#endif
''',
"cpp": r'''#include "wrapped_tokens.h"
namespace shivrai::wrapped_tokens {
WrappedTokens::WrappedTokens() = default;
bool WrappedTokens::Register(const std::string& s, const std::string& c, const std::string& u) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (s.empty() || c.empty() || u.empty()) return false;
    if (assets_.count(s)) return false;
    WrappedAsset a;
    a.symbol = s; a.underlying_chain = c; a.underlying_symbol = u;
    assets_[s] = a;
    return true;
}
bool WrappedTokens::SetActive(const std::string& s, bool act) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = assets_.find(s);
    if (it == assets_.end()) return false;
    it->second.active = act;
    return true;
}
std::optional<WrappedAsset> WrappedTokens::Get(const std::string& s) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = assets_.find(s);
    if (it == assets_.end()) return std::nullopt;
    return it->second;
}
bool WrappedTokens::Wrap(const std::string& s, const std::string& u, uint64_t amt) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = assets_.find(s);
    if (it == assets_.end() || !it->second.active) return false;
    if (u.empty() || amt == 0) return false;
    it->second.total_supply += amt;
    it->second.total_backing += amt;
    balances_[s][u] += amt;
    return true;
}
std::optional<uint64_t> WrappedTokens::Unwrap(const std::string& s, const std::string& u, uint64_t amt) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = assets_.find(s);
    if (it == assets_.end()) return std::nullopt;
    auto bit = balances_[s].find(u);
    if (bit == balances_[s].end() || bit->second < amt) return std::nullopt;
    if (amt == 0) return std::nullopt;
    bit->second -= amt;
    it->second.total_supply -= amt;
    it->second.total_backing -= amt;
    return amt;
}
uint64_t WrappedTokens::BalanceOf(const std::string& s, const std::string& u) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = balances_.find(s);
    if (it == balances_.end()) return 0;
    auto uit = it->second.find(u);
    return uit == it->second.end() ? 0 : uit->second;
}
uint64_t WrappedTokens::TotalSupply(const std::string& s) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = assets_.find(s);
    return it == assets_.end() ? 0 : it->second.total_supply;
}
uint64_t WrappedTokens::TotalBacking(const std::string& s) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = assets_.find(s);
    return it == assets_.end() ? 0 : it->second.total_backing;
}
bool WrappedTokens::IsFullyBacked(const std::string& s) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = assets_.find(s);
    if (it == assets_.end()) return false;
    return it->second.total_backing >= it->second.total_supply;
}
size_t WrappedTokens::AssetCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return assets_.size();
}
}
''',
"test": r'''#include "shivrai/wrapped_tokens/wrapped_tokens.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::wrapped_tokens;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    WrappedTokens wt;
    CHECK(wt.AssetCount() == 0);
    CHECK(!wt.Register("", "eth", "ETH"));
    CHECK(!wt.Register("WETH", "", "ETH"));
    CHECK(!wt.Register("WETH", "eth", ""));
    CHECK(wt.Register("WETH", "ethereum", "ETH"));
    CHECK(!wt.Register("WETH", "eth", "ETH"));
    CHECK(wt.AssetCount() == 1);
    auto a = wt.Get("WETH");
    CHECK(a.has_value());
    CHECK(a->underlying_chain == "ethereum");
    CHECK(a->underlying_symbol == "ETH");
    CHECK(a->total_supply == 0);
    CHECK(wt.IsFullyBacked("WETH"));
    CHECK(!wt.Wrap("NOPE", "alice", 100));
    CHECK(!wt.Wrap("WETH", "", 100));
    CHECK(!wt.Wrap("WETH", "alice", 0));
    CHECK(wt.Wrap("WETH", "alice", 1000));
    CHECK(wt.Wrap("WETH", "bob", 500));
    CHECK(wt.BalanceOf("WETH", "alice") == 1000);
    CHECK(wt.BalanceOf("WETH", "bob") == 500);
    CHECK(wt.BalanceOf("WETH", "carol") == 0);
    CHECK(wt.TotalSupply("WETH") == 1500);
    CHECK(wt.TotalBacking("WETH") == 1500);
    CHECK(wt.IsFullyBacked("WETH"));
    auto u = wt.Unwrap("WETH", "alice", 400);
    CHECK(u.has_value() && *u == 400);
    CHECK(wt.BalanceOf("WETH", "alice") == 600);
    CHECK(wt.TotalSupply("WETH") == 1100);
    CHECK(!wt.Unwrap("WETH", "alice", 99999).has_value());
    CHECK(!wt.Unwrap("NOPE", "alice", 100).has_value());
    CHECK(!wt.Unwrap("WETH", "alice", 0).has_value());
    CHECK(wt.SetActive("WETH", false));
    CHECK(!wt.Wrap("WETH", "carol", 100));
    CHECK(wt.SetActive("WETH", true));
    CHECK(wt.Wrap("WETH", "carol", 100));
    CHECK(!wt.SetActive("NOPE", false));
    CHECK(!wt.Get("NOPE").has_value());
    CHECK(wt.TotalSupply("NOPE") == 0);
    CHECK(wt.TotalBacking("NOPE") == 0);
    CHECK(!wt.IsFullyBacked("NOPE"));
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
'''
}

# ==================== kyc_registry ====================
MODULES["kyc_registry"] = {
"h": r'''#ifndef SHIVRAI_HON_KYC_REGISTRY_H
#define SHIVRAI_HON_KYC_REGISTRY_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
namespace shivrai::kyc_registry {
enum class KycLevel { None = 0, Basic = 1, Verified = 2, Institutional = 3 };
struct KycRecord {
    std::string user;
    KycLevel level{KycLevel::None};
    std::string jurisdiction;
    int64_t verified_at{0};
    int64_t expires_at{0};
    bool revoked{false};
    std::string verifier;
};
class KycRegistry {
public:
    KycRegistry();
    bool Register(const std::string& user, KycLevel level,
                  const std::string& jurisdiction, const std::string& verifier,
                  int64_t validity_sec);
    bool Upgrade(const std::string& user, KycLevel new_level, const std::string& verifier);
    bool Revoke(const std::string& user, const std::string& verifier);
    std::optional<KycRecord> Get(const std::string& user) const;
    bool IsValid(const std::string& user, KycLevel min_level) const;
    KycLevel LevelOf(const std::string& user) const;
    bool SetJurisdiction(const std::string& user, const std::string& jurisdiction);
    size_t Count() const;
    size_t ValidCount(KycLevel min_level) const;
private:
    mutable std::recursive_mutex cs_;
    std::map<std::string, KycRecord> records_;
};
}
#endif
''',
"cpp": r'''#include "kyc_registry.h"
#include <ctime>
namespace shivrai::kyc_registry {
KycRegistry::KycRegistry() = default;
bool KycRegistry::Register(const std::string& u, KycLevel l, const std::string& j,
                           const std::string& v, int64_t val) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (u.empty() || j.empty() || v.empty() || val <= 0) return false;
    if (records_.count(u)) return false;
    KycRecord r;
    r.user = u; r.level = l; r.jurisdiction = j;
    r.verifier = v;
    r.verified_at = (int64_t)std::time(nullptr);
    r.expires_at = r.verified_at + val;
    records_[u] = r;
    return true;
}
bool KycRegistry::Upgrade(const std::string& u, KycLevel nl, const std::string& v) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = records_.find(u);
    if (it == records_.end() || it->second.revoked) return false;
    if (v.empty()) return false;
    if ((int)nl <= (int)it->second.level) return false;
    it->second.level = nl;
    it->second.verifier = v;
    it->second.verified_at = (int64_t)std::time(nullptr);
    return true;
}
bool KycRegistry::Revoke(const std::string& u, const std::string& v) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = records_.find(u);
    if (it == records_.end() || it->second.revoked) return false;
    if (v.empty()) return false;
    it->second.revoked = true;
    return true;
}
std::optional<KycRecord> KycRegistry::Get(const std::string& u) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = records_.find(u);
    if (it == records_.end()) return std::nullopt;
    return it->second;
}
bool KycRegistry::IsValid(const std::string& u, KycLevel ml) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = records_.find(u);
    if (it == records_.end() || it->second.revoked) return false;
    int64_t now = (int64_t)std::time(nullptr);
    if (now >= it->second.expires_at) return false;
    return (int)it->second.level >= (int)ml;
}
KycLevel KycRegistry::LevelOf(const std::string& u) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = records_.find(u);
    if (it == records_.end() || it->second.revoked) return KycLevel::None;
    return it->second.level;
}
bool KycRegistry::SetJurisdiction(const std::string& u, const std::string& j) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = records_.find(u);
    if (it == records_.end() || j.empty()) return false;
    it->second.jurisdiction = j;
    return true;
}
size_t KycRegistry::Count() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return records_.size();
}
size_t KycRegistry::ValidCount(KycLevel ml) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    size_t n = 0;
    for (auto& [k, v] : records_) if (IsValid(k, ml)) ++n;
    return n;
}
}
''',
"test": r'''#include "shivrai/kyc_registry/kyc_registry.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::kyc_registry;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    KycRegistry k;
    CHECK(k.Count() == 0);
    CHECK(!k.Register("", KycLevel::Basic, "IN", "v", 100));
    CHECK(!k.Register("alice", KycLevel::Basic, "", "v", 100));
    CHECK(!k.Register("alice", KycLevel::Basic, "IN", "", 100));
    CHECK(!k.Register("alice", KycLevel::Basic, "IN", "v", 0));
    CHECK(k.Register("alice", KycLevel::Basic, "IN", "verifier1", 3600));
    CHECK(k.Register("bob", KycLevel::Verified, "US", "verifier1", 3600));
    CHECK(k.Count() == 2);
    CHECK(!k.Register("alice", KycLevel::Verified, "IN", "v", 3600));
    auto a = k.Get("alice");
    CHECK(a.has_value());
    CHECK(a->level == KycLevel::Basic);
    CHECK(a->jurisdiction == "IN");
    CHECK(a->verifier == "verifier1");
    CHECK(a->revoked == false);
    CHECK(k.IsValid("alice", KycLevel::Basic));
    CHECK(!k.IsValid("alice", KycLevel::Verified));
    CHECK(k.IsValid("bob", KycLevel::Verified));
    CHECK(k.IsValid("bob", KycLevel::Basic));
    CHECK(k.IsValid("bob", KycLevel::Institutional) == false);
    CHECK(!k.IsValid("carol", KycLevel::Basic));
    CHECK(k.LevelOf("alice") == KycLevel::Basic);
    CHECK(k.LevelOf("bob") == KycLevel::Verified);
    CHECK(k.LevelOf("carol") == KycLevel::None);
    CHECK(k.Upgrade("alice", KycLevel::Verified, "verifier2"));
    CHECK(k.LevelOf("alice") == KycLevel::Verified);
    CHECK(!k.Upgrade("alice", KycLevel::Basic, "v"));  // cannot downgrade
    CHECK(!k.Upgrade("carol", KycLevel::Basic, "v"));
    CHECK(!k.Upgrade("alice", KycLevel::Institutional, ""));
    CHECK(k.SetJurisdiction("alice", "SG"));
    CHECK(k.Get("alice")->jurisdiction == "SG");
    CHECK(!k.SetJurisdiction("alice", ""));
    CHECK(!k.SetJurisdiction("carol", "IN"));
    CHECK(k.Revoke("alice", "verifier1"));
    CHECK(k.Get("alice")->revoked);
    CHECK(!k.IsValid("alice", KycLevel::Basic));
    CHECK(k.LevelOf("alice") == KycLevel::None);
    CHECK(!k.Revoke("alice", "v"));
    CHECK(!k.Upgrade("alice", KycLevel::Institutional, "v"));
    CHECK(k.ValidCount(KycLevel::Basic) == 1);
    CHECK(k.ValidCount(KycLevel::Verified) == 1);
    CHECK(k.ValidCount(KycLevel::Institutional) == 0);
    CHECK(!k.Get("nope").has_value());
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
'''
}

# ==================== ticket_system ====================
MODULES["ticket_system"] = {
"h": r'''#ifndef SHIVRAI_HON_TICKET_SYSTEM_H
#define SHIVRAI_HON_TICKET_SYSTEM_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
namespace shivrai::ticket_system {
using EventId  = uint64_t;
using TicketId = uint64_t;
struct Event {
    EventId id{0};
    std::string name;
    uint64_t capacity{0};
    uint64_t price{0};
    uint64_t sold{0};
    int64_t event_at{0};
    bool active{true};
    bool transferable{true};
};
struct Ticket {
    TicketId id{0};
    EventId event{0};
    std::string owner;
    uint64_t paid{0};
    bool checked_in{false};
    int64_t bought_at{0};
};
class TicketSystem {
public:
    TicketSystem();
    std::optional<EventId> CreateEvent(const std::string& name, uint64_t capacity,
                                       uint64_t price, int64_t event_at);
    bool SetTransferable(EventId id, bool t);
    bool SetActive(EventId id, bool a);
    std::optional<Event> GetEvent(EventId id) const;
    std::optional<TicketId> Buy(EventId id, const std::string& buyer);
    bool CheckIn(TicketId id, const std::string& caller);
    bool Transfer(TicketId id, const std::string& from, const std::string& to);
    std::optional<Ticket> GetTicket(TicketId id) const;
    uint64_t TicketsOwnedBy(const std::string& owner) const;
    size_t EventCount() const;
    size_t TicketCount() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<EventId, Event> events_;
    std::map<TicketId, Ticket> tickets_;
    std::map<std::string, uint64_t> owned_;
    EventId  next_event_id_{1};
    TicketId next_ticket_id_{1};
};
}
#endif
''',
"cpp": r'''#include "ticket_system.h"
#include <ctime>
namespace shivrai::ticket_system {
TicketSystem::TicketSystem() = default;
std::optional<EventId> TicketSystem::CreateEvent(const std::string& n, uint64_t cap,
                                                 uint64_t pr, int64_t ea) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (n.empty() || cap == 0 || pr == 0) return std::nullopt;
    EventId id = next_event_id_++;
    Event e;
    e.id = id; e.name = n; e.capacity = cap; e.price = pr; e.event_at = ea;
    events_[id] = e;
    return id;
}
bool TicketSystem::SetTransferable(EventId id, bool t) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = events_.find(id);
    if (it == events_.end()) return false;
    it->second.transferable = t;
    return true;
}
bool TicketSystem::SetActive(EventId id, bool a) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = events_.find(id);
    if (it == events_.end()) return false;
    it->second.active = a;
    return true;
}
std::optional<Event> TicketSystem::GetEvent(EventId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = events_.find(id);
    if (it == events_.end()) return std::nullopt;
    return it->second;
}
std::optional<TicketId> TicketSystem::Buy(EventId id, const std::string& buyer) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = events_.find(id);
    if (it == events_.end() || !it->second.active) return std::nullopt;
    if (buyer.empty()) return std::nullopt;
    if (it->second.sold >= it->second.capacity) return std::nullopt;
    TicketId tid = next_ticket_id_++;
    Ticket t;
    t.id = tid; t.event = id; t.owner = buyer;
    t.paid = it->second.price;
    t.bought_at = (int64_t)std::time(nullptr);
    tickets_[tid] = t;
    owned_[buyer]++;
    it->second.sold++;
    return tid;
}
bool TicketSystem::CheckIn(TicketId id, const std::string& caller) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = tickets_.find(id);
    if (it == tickets_.end() || it->second.checked_in) return false;
    if (it->second.owner != caller) return false;
    it->second.checked_in = true;
    return true;
}
bool TicketSystem::Transfer(TicketId id, const std::string& from, const std::string& to) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = tickets_.find(id);
    if (it == tickets_.end() || it->second.owner != from) return false;
    if (it->second.checked_in) return false;
    if (to.empty() || to == from) return false;
    auto eit = events_.find(it->second.event);
    if (eit == events_.end() || !eit->second.transferable) return false;
    owned_[from]--;
    owned_[to]++;
    it->second.owner = to;
    return true;
}
std::optional<Ticket> TicketSystem::GetTicket(TicketId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = tickets_.find(id);
    if (it == tickets_.end()) return std::nullopt;
    return it->second;
}
uint64_t TicketSystem::TicketsOwnedBy(const std::string& o) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = owned_.find(o);
    return it == owned_.end() ? 0 : it->second;
}
size_t TicketSystem::EventCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return events_.size();
}
size_t TicketSystem::TicketCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return tickets_.size();
}
}
''',
"test": r'''#include "shivrai/ticket_system/ticket_system.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::ticket_system;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    TicketSystem ts;
    CHECK(ts.EventCount() == 0);
    CHECK(ts.TicketCount() == 0);
    CHECK(!ts.CreateEvent("", 100, 50, 0).has_value());
    CHECK(!ts.CreateEvent("Concert", 0, 50, 0).has_value());
    CHECK(!ts.CreateEvent("Concert", 100, 0, 0).has_value());
    auto e1 = ts.CreateEvent("Concert", 3, 100, 9999999999LL);
    CHECK(e1.has_value());
    CHECK(ts.EventCount() == 1);
    CHECK(ts.GetEvent(*e1)->name == "Concert");
    CHECK(ts.GetEvent(*e1)->capacity == 3);
    CHECK(ts.GetEvent(*e1)->price == 100);
    CHECK(ts.GetEvent(*e1)->sold == 0);
    CHECK(ts.GetEvent(*e1)->active);
    CHECK(ts.GetEvent(*e1)->transferable);
    CHECK(!ts.Buy(*e1, "").has_value());
    CHECK(!ts.Buy(99999, "alice").has_value());
    auto t1 = ts.Buy(*e1, "alice");
    auto t2 = ts.Buy(*e1, "alice");
    auto t3 = ts.Buy(*e1, "bob");
    CHECK(t1.has_value() && t2.has_value() && t3.has_value());
    CHECK(ts.TicketCount() == 3);
    CHECK(ts.GetEvent(*e1)->sold == 3);
    CHECK(ts.TicketsOwnedBy("alice") == 2);
    CHECK(ts.TicketsOwnedBy("bob") == 1);
    CHECK(ts.TicketsOwnedBy("carol") == 0);
    CHECK(!ts.Buy(*e1, "carol").has_value()); // sold out
    auto tk = ts.GetTicket(*t1);
    CHECK(tk.has_value());
    CHECK(tk->owner == "alice");
    CHECK(tk->paid == 100);
    CHECK(!tk->checked_in);
    CHECK(!ts.CheckIn(*t1, "bob"));
    CHECK(ts.CheckIn(*t1, "alice"));
    CHECK(ts.GetTicket(*t1)->checked_in);
    CHECK(!ts.CheckIn(*t1, "alice"));
    CHECK(!ts.Transfer(*t1, "alice", "carol"));  // checked in
    CHECK(ts.Transfer(*t2, "alice", "carol"));
    CHECK(ts.GetTicket(*t2)->owner == "carol");
    CHECK(ts.TicketsOwnedBy("alice") == 1);
    CHECK(ts.TicketsOwnedBy("carol") == 1);
    CHECK(!ts.Transfer(*t2, "alice", "dave"));   // not owner
    CHECK(!ts.Transfer(*t2, "carol", "carol"));  // self
    CHECK(!ts.Transfer(*t2, "carol", ""));       // empty
    ts.SetTransferable(*e1, false);
    CHECK(!ts.Transfer(*t3, "bob", "dave"));
    ts.SetActive(*e1, false);
    CHECK(!ts.Buy(*e1, "dave").has_value());
    CHECK(!ts.SetActive(99999, true));
    CHECK(!ts.SetTransferable(99999, true));
    CHECK(!ts.GetTicket(99999).has_value());
    auto e2 = ts.CreateEvent("Movie", 5, 20, 0);
    CHECK(e2.has_value());
    CHECK(ts.EventCount() == 2);
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
'''
}

# ==================== subscription ====================
MODULES["subscription"] = {
"h": r'''#ifndef SHIVRAI_HON_SUBSCRIPTION_H
#define SHIVRAI_HON_SUBSCRIPTION_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
namespace shivrai::subscription {
using SubId = uint64_t;
struct Sub {
    SubId id{0};
    std::string subscriber;
    std::string provider;
    std::string plan;
    uint64_t amount{0};
    int64_t interval_sec{0};
    int64_t next_charge{0};
    int64_t created_at{0};
    uint64_t cycles_billed{0};
    uint64_t total_billed{0};
    bool active{true};
};
class Subscriptions {
public:
    Subscriptions();
    std::optional<SubId> Subscribe(const std::string& subscriber,
                                   const std::string& provider,
                                   const std::string& plan,
                                   uint64_t amount,
                                   int64_t interval_sec);
    bool Cancel(SubId id, const std::string& caller);
    bool Pause(SubId id, const std::string& caller);
    bool Resume(SubId id, const std::string& caller);
    std::optional<SubId> ChangePlan(SubId id, const std::string& caller,
                                    const std::string& new_plan, uint64_t new_amount);
    uint64_t Charge(SubId id);
    std::optional<Sub> Get(SubId id) const;
    std::vector<SubId> SubsOf(const std::string& subscriber) const;
    size_t Count() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<SubId, Sub> subs_;
    std::map<std::string, std::vector<SubId>> by_subscriber_;
    SubId next_id_{1};
};
}
#endif
''',
"cpp": r'''#include "subscription.h"
#include <ctime>
namespace shivrai::subscription {
Subscriptions::Subscriptions() = default;
std::optional<SubId> Subscriptions::Subscribe(const std::string& s, const std::string& p,
                                              const std::string& pl, uint64_t amt, int64_t iv) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (s.empty() || p.empty() || pl.empty() || amt == 0 || iv <= 0) return std::nullopt;
    if (s == p) return std::nullopt;
    SubId id = next_id_++;
    Sub sub;
    sub.id = id; sub.subscriber = s; sub.provider = p; sub.plan = pl;
    sub.amount = amt; sub.interval_sec = iv;
    sub.created_at = (int64_t)std::time(nullptr);
    sub.next_charge = sub.created_at + iv;
    subs_[id] = sub;
    by_subscriber_[s].push_back(id);
    return id;
}
bool Subscriptions::Cancel(SubId id, const std::string& c) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = subs_.find(id);
    if (it == subs_.end()) return false;
    if (it->second.subscriber != c) return false;
    if (!it->second.active) return false;
    it->second.active = false;
    return true;
}
bool Subscriptions::Pause(SubId id, const std::string& c) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = subs_.find(id);
    if (it == subs_.end() || !it->second.active) return false;
    if (it->second.subscriber != c) return false;
    it->second.active = false;
    return true;
}
bool Subscriptions::Resume(SubId id, const std::string& c) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = subs_.find(id);
    if (it == subs_.end() || it->second.active) return false;
    if (it->second.subscriber != c) return false;
    it->second.active = true;
    return true;
}
std::optional<SubId> Subscriptions::ChangePlan(SubId id, const std::string& c,
                                               const std::string& np, uint64_t na) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = subs_.find(id);
    if (it == subs_.end() || !it->second.active) return std::nullopt;
    if (it->second.subscriber != c) return std::nullopt;
    if (np.empty() || na == 0) return std::nullopt;
    if (it->second.plan == np) return std::nullopt;
    // deactivate old, create new
    it->second.active = false;
    return Subscribe(c, it->second.provider, np, na, it->second.interval_sec);
}
uint64_t Subscriptions::Charge(SubId id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = subs_.find(id);
    if (it == subs_.end() || !it->second.active) return 0;
    it->second.cycles_billed++;
    it->second.total_billed += it->second.amount;
    it->second.next_charge += it->second.interval_sec;
    return it->second.amount;
}
std::optional<Sub> Subscriptions::Get(SubId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = subs_.find(id);
    if (it == subs_.end()) return std::nullopt;
    return it->second;
}
std::vector<SubId> Subscriptions::SubsOf(const std::string& s) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = by_subscriber_.find(s);
    return it == by_subscriber_.end() ? std::vector<SubId>{} : it->second;
}
size_t Subscriptions::Count() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return subs_.size();
}
}
''',
"test": r'''#include "shivrai/subscription/subscription.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::subscription;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    Subscriptions s;
    CHECK(s.Count() == 0);
    CHECK(!s.Subscribe("", "netflix", "basic", 100, 30).has_value());
    CHECK(!s.Subscribe("alice", "", "basic", 100, 30).has_value());
    CHECK(!s.Subscribe("alice", "netflix", "", 100, 30).has_value());
    CHECK(!s.Subscribe("alice", "netflix", "basic", 0, 30).has_value());
    CHECK(!s.Subscribe("alice", "netflix", "basic", 100, 0).has_value());
    CHECK(!s.Subscribe("alice", "alice", "basic", 100, 30).has_value());
    auto s1 = s.Subscribe("alice", "netflix", "basic", 100, 30);
    CHECK(s1.has_value());
    CHECK(s.Count() == 1);
    auto sub = s.Get(*s1);
    CHECK(sub.has_value());
    CHECK(sub->subscriber == "alice");
    CHECK(sub->provider == "netflix");
    CHECK(sub->plan == "basic");
    CHECK(sub->amount == 100);
    CHECK(sub->interval_sec == 30);
    CHECK(sub->active);
    CHECK(sub->cycles_billed == 0);
    CHECK(sub->total_billed == 0);
    CHECK(s.SubsOf("alice").size() == 1);
    CHECK(s.SubsOf("bob").empty());
    auto paid = s.Charge(*s1);
    CHECK(paid == 100);
    CHECK(s.Get(*s1)->cycles_billed == 1);
    CHECK(s.Get(*s1)->total_billed == 100);
    CHECK(s.Charge(*s1) == 100);
    CHECK(s.Get(*s1)->total_billed == 200);
    CHECK(!s.Cancel(*s1, "bob"));
    CHECK(s.Pause(*s1, "alice"));
    CHECK(!s.Get(*s1)->active);
    CHECK(s.Charge(*s1) == 0);
    CHECK(!s.Pause(*s1, "alice"));
    CHECK(s.Resume(*s1, "alice"));
    CHECK(s.Get(*s1)->active);
    CHECK(!s.Resume(*s1, "alice"));
    CHECK(s.Cancel(*s1, "alice"));
    CHECK(!s.Get(*s1)->active);
    CHECK(!s.Cancel(*s1, "alice"));
    CHECK(!s.Get(99999).has_value());
    auto s2 = s.Subscribe("bob", "spotify", "premium", 200, 30);
    CHECK(s2.has_value());
    auto s3 = s.ChangePlan(*s2, "bob", "family", 300);
    CHECK(s3.has_value());
    CHECK(!s.Get(*s2)->active);
    CHECK(s.Get(*s3)->plan == "family");
    CHECK(s.Get(*s3)->amount == 300);
    CHECK(!s.ChangePlan(*s3, "alice", "x", 100).has_value());
    CHECK(!s.ChangePlan(*s3, "bob", "", 100).has_value());
    CHECK(!s.ChangePlan(*s3, "bob", "x", 0).has_value());
    CHECK(!s.ChangePlan(*s3, "bob", "family", 100).has_value());
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
    subprocess.run(["git","-C",ROOT,"commit","-m",f"feat: batch3 ({', '.join(ok)})"])
    subprocess.run(["git","-C",ROOT,"push","origin","master"])
    print("🚀 pushed")

if __name__ == "__main__":
    main()