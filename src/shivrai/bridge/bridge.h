#ifndef SHIVRAI_HON_BRIDGE_H
#define SHIVRAI_HON_BRIDGE_H

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace shivrai::bridge {

using TransferId   = uint64_t;
using ValidatorId  = std::string;

static constexpr uint32_t BPS = 10000;
static constexpr uint32_t DEFAULT_BRIDGE_FEE_BPS = 10;    // 0.1%
static constexpr uint32_t DEFAULT_VALIDATOR_THRESHOLD_BPS = 6667;  // 66.67%
static constexpr int64_t  DEFAULT_CHALLENGE_PERIOD_SEC = 3600;
static constexpr uint64_t MIN_TRANSFER = 100;

enum class TransferState {
    Pending,
    Attesting,
    Ready,
    Executed,
    Cancelled,
    Challenged
};

struct ChainInfo {
    std::string chain_id;
    uint32_t    confirmations_required{12};
    bool        active{true};
};

struct Transfer {
    TransferId    id{0};
    std::string   from_chain;
    std::string   to_chain;
    std::string   sender;
    std::string   recipient;
    std::string   asset;
    uint64_t      amount{0};
    uint64_t      fee{0};
    uint64_t      net_amount{0};
    std::string   source_tx_hash;
    TransferState state{TransferState::Pending};
    std::set<ValidatorId> attestations;
    int64_t       created_at{0};
    int64_t       ready_at{0};
    int64_t       executed_at{0};
    std::string   challenge_reason;
};

class Bridge {
public:
    Bridge();

    // --- Config ---
    void SetBridgeFeeBps(uint32_t bps);
    void SetValidatorThresholdBps(uint32_t bps);
    void SetChallengePeriodSec(int64_t sec);
    uint32_t BridgeFeeBps() const;
    uint32_t ValidatorThresholdBps() const;
    int64_t  ChallengePeriodSec() const;

    // --- Chain registry ---
    bool RegisterChain(const std::string& chain_id, uint32_t confirmations);
    bool SetChainActive(const std::string& chain_id, bool active);
    std::optional<ChainInfo> GetChain(const std::string& chain_id) const;
    std::vector<std::string> ActiveChains() const;
    size_t ChainCount() const;

    // --- Validators ---
    bool AddValidator(const std::string& validator_id, uint64_t power);
    bool RemoveValidator(const std::string& validator_id);
    uint64_t ValidatorPower(const std::string& validator_id) const;
    uint64_t TotalValidatorPower() const;
    size_t   ValidatorCount() const;

    // --- Lock (source chain) ---
    std::optional<TransferId> Lock(const std::string& from_chain,
                                   const std::string& to_chain,
                                   const std::string& sender,
                                   const std::string& recipient,
                                   const std::string& asset,
                                   uint64_t amount,
                                   const std::string& source_tx_hash);

    // --- Attestation (validators observing lock on source chain) ---
    bool Attest(TransferId id, const std::string& validator_id);
    bool RevokeAttestation(TransferId id, const std::string& validator_id);
    uint64_t AttestedPower(TransferId id) const;
    bool     HasAttested(TransferId id, const std::string& validator_id) const;

    // --- Ready & Execute ---
    bool MarkReady(TransferId id);
    bool Execute(TransferId id);
    bool Cancel(TransferId id, const std::string& reason);

    // --- Challenge (fraud proof) ---
    bool Challenge(TransferId id, const std::string& challenger, const std::string& reason);
    bool ResolveChallenge(TransferId id, bool fraudulent);

    // --- Queries ---
    std::optional<Transfer> GetTransfer(TransferId id) const;
    std::vector<TransferId> TransfersByState(TransferState s) const;
    std::vector<TransferId> TransfersTo(const std::string& chain) const;
    size_t TransferCount() const;
    uint64_t TotalLocked(const std::string& chain, const std::string& asset) const;

private:
    mutable std::recursive_mutex cs_;
    std::map<std::string, ChainInfo>      chains_;
    std::map<std::string, uint64_t>       validators_;
    std::map<TransferId, Transfer>        transfers_;
    std::map<std::string, uint64_t>       locked_totals_;   // key = chain|asset

    TransferId next_transfer_id_{1};
    uint32_t   bridge_fee_bps_{DEFAULT_BRIDGE_FEE_BPS};
    uint32_t   validator_threshold_bps_{DEFAULT_VALIDATOR_THRESHOLD_BPS};
    int64_t    challenge_period_sec_{DEFAULT_CHALLENGE_PERIOD_SEC};
};

}
#endif