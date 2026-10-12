#ifndef SHIVRAI_HON_MULTISIG_H
#define SHIVRAI_HON_MULTISIG_H

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace shivrai::multisig {

using WalletId = uint64_t;
using TxId     = uint64_t;

enum class TxState {
    Pending,
    Executed,
    Cancelled
};

struct Wallet {
    WalletId    id{0};
    std::string name;
    std::vector<std::string> owners;
    uint32_t    threshold{0};
    uint64_t    balance{0};
    uint64_t    nonce{0};
    int64_t     created_at{0};
};

struct Transaction {
    TxId        id{0};
    WalletId    wallet{0};
    std::string proposer;
    std::string to;
    uint64_t    amount{0};
    std::string memo;
    uint64_t    nonce{0};
    TxState     state{TxState::Pending};
    std::set<std::string> approvals;
    int64_t     created_at{0};
    int64_t     executed_at{0};
};

class Multisig {
public:
    Multisig();

    // --- Wallet lifecycle ---
    std::optional<WalletId> CreateWallet(const std::string& name,
                                         const std::vector<std::string>& owners,
                                         uint32_t threshold);
    std::optional<Wallet> GetWallet(WalletId id) const;
    size_t WalletCount() const;

    bool AddOwner(WalletId id, const std::string& caller, const std::string& new_owner);
    bool RemoveOwner(WalletId id, const std::string& caller, const std::string& owner);
    bool ChangeThreshold(WalletId id, const std::string& caller, uint32_t new_threshold);

    bool IsOwner(WalletId id, const std::string& who) const;
    uint32_t Threshold(WalletId id) const;
    std::vector<std::string> Owners(WalletId id) const;

    // --- Funding (for tests) ---
    bool Deposit(WalletId id, uint64_t amount);

    // --- Transaction lifecycle ---
    std::optional<TxId> Propose(WalletId id,
                                const std::string& proposer,
                                const std::string& to,
                                uint64_t amount,
                                const std::string& memo);

    bool Approve(WalletId id, TxId tx_id, const std::string& approver);
    bool RevokeApproval(WalletId id, TxId tx_id, const std::string& approver);
    bool Execute(WalletId id, TxId tx_id, const std::string& caller);
    bool Cancel(WalletId id, TxId tx_id, const std::string& caller);

    // --- Queries ---
    std::optional<Transaction> GetTransaction(TxId tx_id) const;
    std::vector<TxId> PendingTxs(WalletId id) const;
    size_t ApprovalCount(TxId tx_id) const;
    bool   HasApproved(TxId tx_id, const std::string& who) const;
    uint64_t Balance(WalletId id) const;
    size_t TransactionCount() const;

private:
    mutable std::recursive_mutex cs_;
    std::map<WalletId, Wallet> wallets_;
    std::map<TxId, Transaction> txs_;
    std::map<WalletId, std::vector<TxId>> wallet_txs_;

    WalletId next_wallet_id_{1};
    TxId     next_tx_id_{1};
};

}
#endif