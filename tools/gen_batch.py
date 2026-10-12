#!/usr/bin/env python3
"""Batch module generator. Adds modules, compiles, tests, commits, pushes."""
import os, subprocess, sys

ROOT = os.path.expanduser("~/shivrai-hon-core")

def write(path, content):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write(content)

MODULES = {}

# ==================== timelock ====================
MODULES["timelock"] = {
"h": r'''#ifndef SHIVRAI_HON_TIMELOCK_H
#define SHIVRAI_HON_TIMELOCK_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
namespace shivrai::timelock {
using OpId = uint64_t;
enum class OpState { Queued, Executed, Cancelled };
struct Operation {
    OpId id{0}; std::string proposer, target, payload;
    int64_t queued_at{0}, eta{0}, executed_at{0};
    OpState state{OpState::Queued};
};
class Timelock {
public:
    Timelock();
    void SetDelaySec(int64_t s);
    int64_t DelaySec() const;
    std::optional<OpId> Queue(const std::string&, const std::string&, const std::string&);
    bool Execute(OpId);
    bool Cancel(OpId, const std::string&);
    std::optional<Operation> Get(OpId) const;
    size_t Count() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<OpId, Operation> ops_;
    OpId next_id_{1};
    int64_t delay_sec_{3600};
};
}
#endif
''',
"cpp": r'''#include "timelock.h"
#include <ctime>
namespace shivrai::timelock {
Timelock::Timelock() = default;
void Timelock::SetDelaySec(int64_t s) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (s >= 0) delay_sec_ = s;
}
int64_t Timelock::DelaySec() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return delay_sec_;
}
std::optional<OpId> Timelock::Queue(const std::string& p, const std::string& t, const std::string& pl) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (p.empty() || t.empty()) return std::nullopt;
    OpId id = next_id_++;
    Operation op;
    op.id = id; op.proposer = p; op.target = t; op.payload = pl;
    op.queued_at = (int64_t)std::time(nullptr);
    op.eta = op.queued_at + delay_sec_;
    ops_[id] = op;
    return id;
}
bool Timelock::Execute(OpId id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = ops_.find(id);
    if (it == ops_.end()) return false;
    if (it->second.state != OpState::Queued) return false;
    int64_t now = (int64_t)std::time(nullptr);
    if (now < it->second.eta) return false;
    it->second.executed_at = now;
    it->second.state = OpState::Executed;
    return true;
}
bool Timelock::Cancel(OpId id, const std::string& c) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = ops_.find(id);
    if (it == ops_.end()) return false;
    if (it->second.state != OpState::Queued) return false;
    if (it->second.proposer != c) return false;
    it->second.state = OpState::Cancelled;
    return true;
}
std::optional<Operation> Timelock::Get(OpId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = ops_.find(id);
    if (it == ops_.end()) return std::nullopt;
    return it->second;
}
size_t Timelock::Count() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return ops_.size();
}
}
''',
"test": r'''#include "shivrai/timelock/timelock.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::timelock;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    Timelock tl; tl.SetDelaySec(0);
    CHECK(tl.DelaySec() == 0);
    CHECK(tl.Count() == 0);
    auto o = tl.Queue("alice", "setFee", "0x01");
    CHECK(o.has_value());
    CHECK(tl.Count() == 1);
    CHECK(tl.Get(*o)->proposer == "alice");
    CHECK(tl.Execute(*o));
    CHECK(tl.Get(*o)->state == OpState::Executed);
    CHECK(!tl.Execute(*o));
    CHECK(!tl.Queue("", "x", "y").has_value());
    CHECK(!tl.Get(99999).has_value());
    auto o2 = tl.Queue("bob", "upgrade", "0x02");
    CHECK(!tl.Cancel(*o2, "alice"));
    CHECK(tl.Cancel(*o2, "bob"));
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
'''
}

# ==================== analytics ====================
MODULES["analytics"] = {
"h": r'''#ifndef SHIVRAI_HON_ANALYTICS_H
#define SHIVRAI_HON_ANALYTICS_H
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>
namespace shivrai::analytics {
struct Series { std::string name; std::vector<std::pair<int64_t,uint64_t>> points; };
class Analytics {
public:
    Analytics();
    bool Record(const std::string& series, uint64_t value);
    bool RecordAt(const std::string& series, int64_t ts, uint64_t value);
    uint64_t Sum(const std::string& series) const;
    uint64_t Count(const std::string& series) const;
    uint64_t Max(const std::string& series) const;
    uint64_t Min(const std::string& series) const;
    double Avg(const std::string& series) const;
    std::vector<std::pair<int64_t,uint64_t>> Points(const std::string& series) const;
    std::vector<std::string> AllSeries() const;
    size_t SeriesCount() const;
    bool Clear(const std::string& series);
private:
    mutable std::recursive_mutex cs_;
    std::map<std::string, Series> data_;
};
}
#endif
''',
"cpp": r'''#include "analytics.h"
#include <algorithm>
#include <ctime>
namespace shivrai::analytics {
Analytics::Analytics() = default;
bool Analytics::Record(const std::string& s, uint64_t v) {
    return RecordAt(s, (int64_t)std::time(nullptr), v);
}
bool Analytics::RecordAt(const std::string& s, int64_t ts, uint64_t v) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (s.empty()) return false;
    data_[s].name = s;
    data_[s].points.emplace_back(ts, v);
    return true;
}
uint64_t Analytics::Sum(const std::string& s) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = data_.find(s);
    if (it == data_.end()) return 0;
    uint64_t t = 0;
    for (auto& p : it->second.points) t += p.second;
    return t;
}
uint64_t Analytics::Count(const std::string& s) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = data_.find(s);
    return it == data_.end() ? 0 : it->second.points.size();
}
uint64_t Analytics::Max(const std::string& s) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = data_.find(s);
    if (it == data_.end() || it->second.points.empty()) return 0;
    uint64_t m = 0;
    for (auto& p : it->second.points) m = std::max(m, p.second);
    return m;
}
uint64_t Analytics::Min(const std::string& s) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = data_.find(s);
    if (it == data_.end() || it->second.points.empty()) return 0;
    uint64_t m = UINT64_MAX;
    for (auto& p : it->second.points) m = std::min(m, p.second);
    return m;
}
double Analytics::Avg(const std::string& s) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = data_.find(s);
    if (it == data_.end() || it->second.points.empty()) return 0.0;
    uint64_t sum = 0;
    for (auto& p : it->second.points) sum += p.second;
    return (double)sum / it->second.points.size();
}
std::vector<std::pair<int64_t,uint64_t>> Analytics::Points(const std::string& s) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = data_.find(s);
    return it == data_.end() ? std::vector<std::pair<int64_t,uint64_t>>{} : it->second.points;
}
std::vector<std::string> Analytics::AllSeries() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<std::string> out;
    for (auto& [k, v] : data_) out.push_back(k);
    return out;
}
size_t Analytics::SeriesCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return data_.size();
}
bool Analytics::Clear(const std::string& s) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return data_.erase(s) > 0;
}
}
''',
"test": r'''#include "shivrai/analytics/analytics.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
using namespace shivrai::analytics;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    Analytics a;
    CHECK(a.SeriesCount() == 0);
    CHECK(a.Sum("x") == 0);
    CHECK(a.Count("x") == 0);
    CHECK(!a.Record("", 10));
    CHECK(a.Record("tx", 100));
    CHECK(a.Record("tx", 200));
    CHECK(a.Record("tx", 300));
    CHECK(a.SeriesCount() == 1);
    CHECK(a.Count("tx") == 3);
    CHECK(a.Sum("tx") == 600);
    CHECK(a.Max("tx") == 300);
    CHECK(a.Min("tx") == 100);
    CHECK(std::fabs(a.Avg("tx") - 200.0) < 1e-9);
    CHECK(a.Points("tx").size() == 3);
    a.Record("gas", 50);
    a.Record("gas", 70);
    CHECK(a.AllSeries().size() == 2);
    CHECK(a.Clear("gas"));
    CHECK(a.SeriesCount() == 1);
    CHECK(!a.Clear("gas"));
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
'''
}

# ==================== event_log ====================
MODULES["event_log"] = {
"h": r'''#ifndef SHIVRAI_HON_EVENT_LOG_H
#define SHIVRAI_HON_EVENT_LOG_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
namespace shivrai::event_log {
using EventId = uint64_t;
struct Event { EventId id{0}; std::string topic, payload; int64_t ts{0}; };
class EventLog {
public:
    EventLog();
    EventId Emit(const std::string& topic, const std::string& payload);
    std::optional<Event> Get(EventId) const;
    std::vector<Event> ByTopic(const std::string& topic) const;
    std::vector<Event> Range(EventId from, EventId to) const;
    size_t Count() const;
    size_t TopicCount() const;
    EventId LastId() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<EventId, Event> events_;
    std::map<std::string, std::vector<EventId>> by_topic_;
    EventId next_id_{1};
};
}
#endif
''',
"cpp": r'''#include "event_log.h"
#include <ctime>
namespace shivrai::event_log {
EventLog::EventLog() = default;
EventId EventLog::Emit(const std::string& t, const std::string& p) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (t.empty()) return 0;
    EventId id = next_id_++;
    Event e;
    e.id = id; e.topic = t; e.payload = p;
    e.ts = (int64_t)std::time(nullptr);
    events_[id] = e;
    by_topic_[t].push_back(id);
    return id;
}
std::optional<Event> EventLog::Get(EventId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = events_.find(id);
    if (it == events_.end()) return std::nullopt;
    return it->second;
}
std::vector<Event> EventLog::ByTopic(const std::string& t) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<Event> out;
    auto it = by_topic_.find(t);
    if (it == by_topic_.end()) return out;
    for (auto id : it->second) {
        auto e = events_.find(id);
        if (e != events_.end()) out.push_back(e->second);
    }
    return out;
}
std::vector<Event> EventLog::Range(EventId a, EventId b) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<Event> out;
    for (auto& [id, e] : events_) if (id >= a && id <= b) out.push_back(e);
    return out;
}
size_t EventLog::Count() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return events_.size();
}
size_t EventLog::TopicCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return by_topic_.size();
}
EventId EventLog::LastId() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return events_.empty() ? 0 : events_.rbegin()->first;
}
}
''',
"test": r'''#include "shivrai/event_log/event_log.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::event_log;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    EventLog l;
    CHECK(l.Count() == 0);
    CHECK(l.LastId() == 0);
    CHECK(l.Emit("", "x") == 0);
    auto a = l.Emit("transfer", "alice->bob 100");
    auto b = l.Emit("mint", "carol +50");
    auto c = l.Emit("transfer", "dave->eve 20");
    CHECK(a == 1 && b == 2 && c == 3);
    CHECK(l.Count() == 3);
    CHECK(l.TopicCount() == 2);
    CHECK(l.LastId() == 3);
    CHECK(l.Get(a).has_value());
    CHECK(l.Get(a)->topic == "transfer");
    CHECK(!l.Get(99999).has_value());
    CHECK(l.ByTopic("transfer").size() == 2);
    CHECK(l.ByTopic("nope").empty());
    CHECK(l.Range(1, 2).size() == 2);
    CHECK(l.Range(2, 3).size() == 2);
    CHECK(l.Range(99, 99).empty());
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
'''
}

# ==================== payment_stream ====================
MODULES["payment_stream"] = {
"h": r'''#ifndef SHIVRAI_HON_PAYMENT_STREAM_H
#define SHIVRAI_HON_PAYMENT_STREAM_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
namespace shivrai::payment_stream {
using StreamId = uint64_t;
enum class StreamState { Active, Paused, Cancelled, Completed };
struct Stream {
    StreamId id{0};
    std::string sender, recipient;
    uint64_t total{0}, withdrawn{0};
    int64_t start{0}, end{0};
    StreamState state{StreamState::Active};
};
class PaymentStream {
public:
    PaymentStream();
    std::optional<StreamId> Create(const std::string& sender, const std::string& recipient,
                                   uint64_t total, int64_t duration_sec);
    uint64_t Claimable(StreamId) const;
    std::optional<uint64_t> Withdraw(StreamId);
    bool Pause(StreamId, const std::string& caller);
    bool Resume(StreamId, const std::string& caller);
    bool Cancel(StreamId, const std::string& caller);
    std::optional<Stream> Get(StreamId) const;
    size_t Count() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<StreamId, Stream> streams_;
    StreamId next_id_{1};
};
}
#endif
''',
"cpp": r'''#include "payment_stream.h"
#include <algorithm>
#include <ctime>
namespace shivrai::payment_stream {
PaymentStream::PaymentStream() = default;
std::optional<StreamId> PaymentStream::Create(const std::string& s, const std::string& r,
                                              uint64_t total, int64_t dur) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (s.empty() || r.empty() || s == r || total == 0 || dur <= 0) return std::nullopt;
    StreamId id = next_id_++;
    Stream st;
    st.id = id; st.sender = s; st.recipient = r;
    st.total = total;
    st.start = (int64_t)std::time(nullptr);
    st.end = st.start + dur;
    streams_[id] = st;
    return id;
}
uint64_t PaymentStream::Claimable(StreamId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = streams_.find(id);
    if (it == streams_.end()) return 0;
    if (it->second.state == StreamState::Cancelled) return 0;
    int64_t now = (int64_t)std::time(nullptr);
    int64_t elapsed = now - it->second.start;
    int64_t dur = it->second.end - it->second.start;
    if (elapsed <= 0) return 0;
    if (elapsed >= dur) return it->second.total - it->second.withdrawn;
    uint64_t vested = (uint64_t)((__uint128_t)it->second.total * elapsed / dur);
    return vested > it->second.withdrawn ? vested - it->second.withdrawn : 0;
}
std::optional<uint64_t> PaymentStream::Withdraw(StreamId id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = streams_.find(id);
    if (it == streams_.end()) return std::nullopt;
    if (it->second.state == StreamState::Cancelled) return std::nullopt;
    uint64_t c = Claimable(id);
    if (c == 0) return std::nullopt;
    it->second.withdrawn += c;
    if (it->second.withdrawn >= it->second.total) it->second.state = StreamState::Completed;
    return c;
}
bool PaymentStream::Pause(StreamId id, const std::string& c) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = streams_.find(id);
    if (it == streams_.end()) return false;
    if (it->second.sender != c) return false;
    if (it->second.state != StreamState::Active) return false;
    it->second.state = StreamState::Paused;
    return true;
}
bool PaymentStream::Resume(StreamId id, const std::string& c) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = streams_.find(id);
    if (it == streams_.end()) return false;
    if (it->second.sender != c) return false;
    if (it->second.state != StreamState::Paused) return false;
    it->second.state = StreamState::Active;
    return true;
}
bool PaymentStream::Cancel(StreamId id, const std::string& c) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = streams_.find(id);
    if (it == streams_.end()) return false;
    if (it->second.sender != c) return false;
    if (it->second.state == StreamState::Cancelled) return false;
    if (it->second.state == StreamState::Completed) return false;
    it->second.state = StreamState::Cancelled;
    return true;
}
std::optional<Stream> PaymentStream::Get(StreamId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = streams_.find(id);
    if (it == streams_.end()) return std::nullopt;
    return it->second;
}
size_t PaymentStream::Count() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return streams_.size();
}
}
''',
"test": r'''#include "shivrai/payment_stream/payment_stream.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::payment_stream;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    PaymentStream ps;
    CHECK(ps.Count() == 0);
    CHECK(!ps.Create("", "b", 100, 100).has_value());
    CHECK(!ps.Create("a", "b", 0, 100).has_value());
    CHECK(!ps.Create("a", "b", 100, 0).has_value());
    CHECK(!ps.Create("a", "a", 100, 100).has_value());
    auto s1 = ps.Create("alice", "bob", 1000, 100);
    CHECK(s1.has_value());
    CHECK(ps.Count() == 1);
    auto st = ps.Get(*s1);
    CHECK(st.has_value());
    CHECK(st->sender == "alice");
    CHECK(st->recipient == "bob");
    CHECK(st->total == 1000);
    CHECK(st->state == StreamState::Active);
    CHECK(ps.Claimable(*s1) == 0);
    CHECK(!ps.Withdraw(*s1).has_value());
    CHECK(!ps.Get(99999).has_value());
    CHECK(!ps.Pause(99999, "alice"));
    CHECK(!ps.Pause(*s1, "bob"));
    CHECK(ps.Pause(*s1, "alice"));
    CHECK(ps.Get(*s1)->state == StreamState::Paused);
    CHECK(!ps.Pause(*s1, "alice"));
    CHECK(ps.Resume(*s1, "alice"));
    CHECK(!ps.Resume(*s1, "alice"));
    CHECK(ps.Cancel(*s1, "alice"));
    CHECK(ps.Get(*s1)->state == StreamState::Cancelled);
    CHECK(!ps.Cancel(*s1, "alice"));
    CHECK(ps.Claimable(*s1) == 0);
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
    subprocess.run(["git","-C",ROOT,"commit","-m",f"feat: batch modules ({', '.join(ok)})"])
    subprocess.run(["git","-C",ROOT,"push","origin","master"])
    print("🚀 pushed")

if __name__ == "__main__":
    main()
