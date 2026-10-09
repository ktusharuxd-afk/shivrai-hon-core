#include "message.h"
#include "shivrai/common/time.hpp"
#include <random>
#include <sstream>
#include <mutex>

namespace shivrai::interop {

std::string MessageRelay::gen_id() const {
    static std::mutex rng_mutex;
    static std::mt19937_64 rng(shivrai::common::monotonic_ns());
    std::lock_guard<std::mutex> rng_lock(rng_mutex);

    std::ostringstream ss;
    ss << "MSG_" << rng();
    return ss.str();
}

MessageRelay::MessageRelay(uint32_t min_relayers) : min_relayers_(min_relayers) {}

std::string MessageRelay::send_message(ChainId src, ChainId dst,
                                        const std::string& sender,
                                        const std::string& receiver,
                                        const std::string& payload,
                                        uint64_t amount,
                                        const std::string& asset) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (sender.empty() || receiver.empty()) return "";
    CrossChainMessage m;
    m.id = gen_id();
    m.src_chain = src;
    m.dst_chain = dst;
    m.sender = sender;
    m.receiver = receiver;
    m.payload = payload;
    m.amount = amount;
    m.asset = asset;
    m.nonce = ++nonce_;
    m.status = MessageStatus::PENDING;
    m.created_at = shivrai::common::now_seconds();
    messages_[m.id] = m;
    return m.id;
}

bool MessageRelay::relay_message(const std::string& msg_id,
                                  const std::string& relayer,
                                  const std::string& signature) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = messages_.find(msg_id);
    if (it == messages_.end()) return false;
    if (it->second.status != MessageStatus::PENDING) return false;
    RelayProof p;
    p.message_id = msg_id;
    p.relayer = relayer;
    p.signature = signature;
    p.relayed_at = shivrai::common::now_seconds();
    proofs_[msg_id].push_back(p);
    if ((uint32_t)proofs_[msg_id].size() >= min_relayers_) {
        it->second.status = MessageStatus::RELAYED;
    }
    return true;
}

bool MessageRelay::confirm_message(const std::string& msg_id) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = messages_.find(msg_id);
    if (it == messages_.end()) return false;
    it->second.status = MessageStatus::CONFIRMED;
    if (on_confirmed_) on_confirmed_(it->second);
    return true;
}

bool MessageRelay::fail_message(const std::string& msg_id) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = messages_.find(msg_id);
    if (it == messages_.end()) return false;
    it->second.status = MessageStatus::FAILED;
    return true;
}

const CrossChainMessage* MessageRelay::get_message(const std::string& id) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = messages_.find(id);
    return it == messages_.end() ? nullptr : &it->second;
}

MessageStatus MessageRelay::status(const std::string& id) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto m = get_message(id);
    return m ? m->status : MessageStatus::FAILED;
}

std::vector<CrossChainMessage> MessageRelay::pending_messages(ChainId dst) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<CrossChainMessage> result;
    for (const auto& [id, m] : messages_)
        if (m.dst_chain == dst && m.status == MessageStatus::PENDING)
            result.push_back(m);
    return result;
}

} // namespace shivrai::interop
