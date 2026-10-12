#ifndef SHIVRAI_HON_ATOMIC_SWAP_H
#define SHIVRAI_HON_ATOMIC_SWAP_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
namespace shivrai::atomic_swap {
using SwapId = uint64_t;
enum class SwapState { Open, Claimed, Refunded };
struct HtlcSwap {
    SwapId id{0};
    std::string initiator, participant;
    std::string from_chain, to_chain;
    std::string asset;
    uint64_t amount{0};
    std::string hash_lock;      // H = hash(secret)
    std::string secret;         // revealed on claim
    int64_t timeout{0};
    int64_t created_at{0};
    SwapState state{SwapState::Open};
};
class AtomicSwap {
public:
    AtomicSwap();
    std::optional<SwapId> Initiate(const std::string& initiator,
                                   const std::string& participant,
                                   const std::string& from_chain,
                                   const std::string& to_chain,
                                   const std::string& asset,
                                   uint64_t amount,
                                   const std::string& hash_lock,
                                   int64_t timeout_sec);
    bool Claim(SwapId id, const std::string& caller, const std::string& secret);
    bool Refund(SwapId id, const std::string& caller);
    std::optional<HtlcSwap> Get(SwapId id) const;
    size_t Count() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<SwapId, HtlcSwap> swaps_;
    SwapId next_id_{1};
};
}
#endif
