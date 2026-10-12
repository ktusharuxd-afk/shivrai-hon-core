#include "atomic_swap.h"
#include <ctime>
namespace shivrai::atomic_swap {
AtomicSwap::AtomicSwap() = default;
std::optional<SwapId> AtomicSwap::Initiate(const std::string& ini, const std::string& p,
                                           const std::string& fc, const std::string& tc,
                                           const std::string& a, uint64_t amt,
                                           const std::string& hl, int64_t tos) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (ini.empty() || p.empty() || ini == p) return std::nullopt;
    if (fc.empty() || tc.empty() || fc == tc) return std::nullopt;
    if (a.empty() || amt == 0 || hl.empty() || tos <= 0) return std::nullopt;
    SwapId id = next_id_++;
    HtlcSwap s;
    s.id = id; s.initiator = ini; s.participant = p;
    s.from_chain = fc; s.to_chain = tc; s.asset = a; s.amount = amt;
    s.hash_lock = hl;
    s.created_at = (int64_t)std::time(nullptr);
    s.timeout = s.created_at + tos;
    swaps_[id] = s;
    return id;
}
bool AtomicSwap::Claim(SwapId id, const std::string& caller, const std::string& secret) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = swaps_.find(id);
    if (it == swaps_.end()) return false;
    if (it->second.state != SwapState::Open) return false;
    if (it->second.participant != caller) return false;
    if (secret.empty()) return false;
    int64_t now = (int64_t)std::time(nullptr);
    if (now >= it->second.timeout) return false;
    it->second.secret = secret;
    it->second.state = SwapState::Claimed;
    return true;
}
bool AtomicSwap::Refund(SwapId id, const std::string& caller) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = swaps_.find(id);
    if (it == swaps_.end()) return false;
    if (it->second.state != SwapState::Open) return false;
    if (it->second.initiator != caller) return false;
    int64_t now = (int64_t)std::time(nullptr);
    if (now < it->second.timeout) return false;
    it->second.state = SwapState::Refunded;
    return true;
}
std::optional<HtlcSwap> AtomicSwap::Get(SwapId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = swaps_.find(id);
    if (it == swaps_.end()) return std::nullopt;
    return it->second;
}
size_t AtomicSwap::Count() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return swaps_.size();
}
}
