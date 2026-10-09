#ifndef SHIVRAI_HON_INTEROP_H
#define SHIVRAI_HON_INTEROP_H
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <functional>
#include <mutex>

namespace shivrai::interop {

enum class MessageStatus { PENDING, RELAYED, CONFIRMED, FAILED };
enum class ChainId { SHIVRAI=0, ETHEREUM=1, BNB=2, POLYGON=3, SOLANA=4 };

struct CrossChainMessage {
    std::string id;
    ChainId src_chain;
    ChainId dst_chain;
    std::string sender;
    std::string receiver;
    std::string payload;
    uint64_t amount{0};
    std::string asset;
    uint64_t nonce{0};
    MessageStatus status{MessageStatus::PENDING};
    uint64_t created_at{0};
    std::string tx_hash;
};

struct RelayProof {
    std::string message_id;
    std::string relayer;
    std::string signature;
    uint64_t relayed_at{0};
};

class MessageRelay {
public:
    explicit MessageRelay(uint32_t min_relayers = 2);

    std::string send_message(ChainId src, ChainId dst,
                              const std::string& sender,
                              const std::string& receiver,
                              const std::string& payload,
                              uint64_t amount = 0,
                              const std::string& asset = "");

    bool relay_message(const std::string& msg_id,
                        const std::string& relayer,
                        const std::string& signature);

    bool confirm_message(const std::string& msg_id);
    bool fail_message(const std::string& msg_id);

    const CrossChainMessage* get_message(const std::string& id) const;
    MessageStatus status(const std::string& id) const;
    std::vector<CrossChainMessage> pending_messages(ChainId dst) const;

    using MessageCallback = std::function<void(const CrossChainMessage&)>;
    void on_confirmed(MessageCallback cb) { on_confirmed_ = cb; }

    size_t message_count() const { return messages_.size(); }

private:
    uint32_t min_relayers_;
    uint64_t nonce_{0};
    mutable std::recursive_mutex mutex_;
    std::unordered_map<std::string, CrossChainMessage> messages_;
    std::unordered_map<std::string, std::vector<RelayProof>> proofs_;
    MessageCallback on_confirmed_;
    std::string gen_id() const;
};

} // namespace shivrai::interop
#endif
