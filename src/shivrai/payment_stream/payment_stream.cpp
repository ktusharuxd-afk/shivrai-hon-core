#include "payment_stream.h"
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
