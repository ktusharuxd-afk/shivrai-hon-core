#include "bridge.h"
#include <algorithm>
#include <ctime>

namespace shivrai::bridge {

Bridge::Bridge() = default;

// ---- Config ----
void Bridge::SetBridgeFeeBps(uint32_t bps) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (bps <= BPS) bridge_fee_bps_ = bps;
}
void Bridge::SetValidatorThresholdBps(uint32_t bps) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (bps > 0 && bps <= BPS) validator_threshold_bps_ = bps;
}
void Bridge::SetChallengePeriodSec(int64_t sec) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (sec >= 0) challenge_period_sec_ = sec;
}
uint32_t Bridge::BridgeFeeBps() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return bridge_fee_bps_;
}
uint32_t Bridge::ValidatorThresholdBps() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return validator_threshold_bps_;
}
int64_t Bridge::ChallengePeriodSec() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return challenge_period_sec_;
}

// ---- Chain registry ----
bool Bridge::RegisterChain(const std::string& chain_id, uint32_t confirmations) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (chain_id.empty()) return false;
    if (chains_.count(chain_id)) return false;
    ChainInfo c;
    c.chain_id = chain_id;
    c.confirmations_required = confirmations;
    c.active = true;
    chains_[chain_id] = c;
    return true;
}

bool Bridge::SetChainActive(const std::string& chain_id, bool active) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = chains_.find(chain_id);
    if (it == chains_.end()) return false;
    it->second.active = active;
    return true;
}

std::optional<ChainInfo> Bridge::GetChain(const std::string& chain_id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = chains_.find(chain_id);
    if (it == chains_.end()) return std::nullopt;
    return it->second;
}

std::vector<std::string> Bridge::ActiveChains() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<std::string> out;
    for (auto& [k, v] : chains_) if (v.active) out.push_back(k);
    return out;
}

size_t Bridge::ChainCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return chains_.size();
}

// ---- Validators ----
bool Bridge::AddValidator(const std::string& validator_id, uint64_t power) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (validator_id.empty() || power == 0) return false;
    if (validators_.count(validator_id)) return false;
    validators_[validator_id] = power;
    return true;
}

bool Bridge::RemoveValidator(const std::string& validator_id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return validators_.erase(validator_id) > 0;
}

uint64_t Bridge::ValidatorPower(const std::string& validator_id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = validators_.find(validator_id);
    return it == validators_.end() ? 0 : it->second;
}

uint64_t Bridge::TotalValidatorPower() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    uint64_t t = 0;
    for (auto& [k, v] : validators_) t += v;
    return t;
}

size_t Bridge::ValidatorCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return validators_.size();
}

// ---- Lock (source chain) ----
std::optional<TransferId> Bridge::Lock(const std::string& from_chain,
                                       const std::string& to_chain,
                                       const std::string& sender,
                                       const std::string& recipient,
                                       const std::string& asset,
                                       uint64_t amount,
                                       const std::string& source_tx_hash) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (from_chain.empty() || to_chain.empty() || from_chain == to_chain) return std::nullopt;
    if (sender.empty() || recipient.empty() || asset.empty()) return std::nullopt;
    if (source_tx_hash.empty()) return std::nullopt;
    if (amount < MIN_TRANSFER) return std::nullopt;

    auto fit = chains_.find(from_chain);
    auto tit = chains_.find(to_chain);
    if (fit == chains_.end() || tit == chains_.end()) return std::nullopt;
    if (!fit->second.active || !tit->second.active) return std::nullopt;

    TransferId id = next_transfer_id_++;
    Transfer t;
    t.id = id;
    t.from_chain = from_chain;
    t.to_chain = to_chain;
    t.sender = sender;
    t.recipient = recipient;
    t.asset = asset;
    t.amount = amount;
    t.fee = (uint64_t)((__uint128_t)amount * bridge_fee_bps_ / BPS);
    t.net_amount = amount - t.fee;
    t.source_tx_hash = source_tx_hash;
    t.state = TransferState::Pending;
    t.created_at = (int64_t)std::time(nullptr);

    transfers_[id] = t;
    locked_totals_[from_chain + "|" + asset] += amount;
    return id;
}

// ---- Attestation ----
bool Bridge::Attest(TransferId id, const std::string& validator_id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = transfers_.find(id);
    if (it == transfers_.end()) return false;
    if (it->second.state != TransferState::Pending &&
        it->second.state != TransferState::Attesting) return false;
    if (validators_.find(validator_id) == validators_.end()) return false;
    if (it->second.attestations.count(validator_id)) return false;

    it->second.attestations.insert(validator_id);
    it->second.state = TransferState::Attesting;
    return true;
}

bool Bridge::RevokeAttestation(TransferId id, const std::string& validator_id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = transfers_.find(id);
    if (it == transfers_.end()) return false;
    if (it->second.state != TransferState::Attesting) return false;
    if (!it->second.attestations.erase(validator_id)) return false;
    if (it->second.attestations.empty()) it->second.state = TransferState::Pending;
    return true;
}

uint64_t Bridge::AttestedPower(TransferId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = transfers_.find(id);
    if (it == transfers_.end()) return 0;
    uint64_t p = 0;
    for (auto& v : it->second.attestations) {
        auto vit = validators_.find(v);
        if (vit != validators_.end()) p += vit->second;
    }
    return p;
}

bool Bridge::HasAttested(TransferId id, const std::string& validator_id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = transfers_.find(id);
    if (it == transfers_.end()) return false;
    return it->second.attestations.count(validator_id) > 0;
}

// ---- Ready & Execute ----
bool Bridge::MarkReady(TransferId id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = transfers_.find(id);
    if (it == transfers_.end()) return false;
    if (it->second.state != TransferState::Attesting) return false;
    uint64_t total = TotalValidatorPower();
    if (total == 0) return false;
    uint64_t attested = AttestedPower(id);
    __uint128_t pct = (__uint128_t)attested * BPS / total;
    if ((uint64_t)pct < validator_threshold_bps_) return false;
    it->second.state = TransferState::Ready;
    it->second.ready_at = (int64_t)std::time(nullptr);
    return true;
}

bool Bridge::Execute(TransferId id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = transfers_.find(id);
    if (it == transfers_.end()) return false;
    if (it->second.state != TransferState::Ready) return false;
    int64_t now = (int64_t)std::time(nullptr);
    if (challenge_period_sec_ > 0 &&
        now - it->second.ready_at < challenge_period_sec_) return false;
    it->second.state = TransferState::Executed;
    it->second.executed_at = now;
    return true;
}

bool Bridge::Cancel(TransferId id, const std::string& reason) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = transfers_.find(id);
    if (it == transfers_.end()) return false;
    if (it->second.state == TransferState::Executed) return false;
    if (it->second.state == TransferState::Cancelled) return false;
    it->second.state = TransferState::Cancelled;
    it->second.challenge_reason = reason;
    return true;
}

// ---- Challenge ----
bool Bridge::Challenge(TransferId id, const std::string& challenger, const std::string& reason) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = transfers_.find(id);
    if (it == transfers_.end()) return false;
    if (it->second.state == TransferState::Executed) return false;
    if (it->second.state == TransferState::Cancelled) return false;
    if (it->second.state == TransferState::Challenged) return false;
    if (challenger.empty()) return false;
    it->second.state = TransferState::Challenged;
    it->second.challenge_reason = reason;
    return true;
}

bool Bridge::ResolveChallenge(TransferId id, bool fraudulent) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = transfers_.find(id);
    if (it == transfers_.end()) return false;
    if (it->second.state != TransferState::Challenged) return false;
    if (fraudulent) {
        it->second.state = TransferState::Cancelled;
    } else {
        // restore based on attestation count
        it->second.state = it->second.attestations.empty()
                            ? TransferState::Pending
                            : TransferState::Attesting;
    }
    return true;
}

// ---- Queries ----
std::optional<Transfer> Bridge::GetTransfer(TransferId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = transfers_.find(id);
    if (it == transfers_.end()) return std::nullopt;
    return it->second;
}

std::vector<TransferId> Bridge::TransfersByState(TransferState s) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<TransferId> out;
    for (auto& [id, t] : transfers_) if (t.state == s) out.push_back(id);
    return out;
}

std::vector<TransferId> Bridge::TransfersTo(const std::string& chain) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<TransferId> out;
    for (auto& [id, t] : transfers_) if (t.to_chain == chain) out.push_back(id);
    return out;
}

size_t Bridge::TransferCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return transfers_.size();
}

uint64_t Bridge::TotalLocked(const std::string& chain, const std::string& asset) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = locked_totals_.find(chain + "|" + asset);
    return it == locked_totals_.end() ? 0 : it->second;
}

}