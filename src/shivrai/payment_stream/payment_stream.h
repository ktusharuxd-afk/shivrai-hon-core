#ifndef SHIVRAI_HON_PAYMENT_STREAM_H
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
