#include "multisig.h"
#include <algorithm>
#include <ctime>

namespace shivrai::multisig {

Multisig::Multisig() = default;

// ---- Wallet lifecycle ----
std::optional<WalletId> Multisig::CreateWallet(const std::string& name,
                                               const std::vector<std::string>& owners,
                                               uint32_t threshold) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (name.empty() || owners.empty()) return std::nullopt;
    if (threshold == 0 || threshold > owners.size()) return std::nullopt;

    // dedupe owners
    std::set<std::string> unique;
    for (auto& o : owners) {
        if (o.empty()) return std::nullopt;
        unique.insert(o);
    }
    if (unique.size() != owners.size()) return std::nullopt;
    if (threshold > unique.size()) return std::nullopt;

    WalletId id = next_wallet_id_++;
    Wallet w;
    w.id = id;
    w.name = name;
    w.owners.assign(unique.begin(), unique.end());
    w.threshold = threshold;
    w.created_at = (int64_t)std::time(nullptr);
    wallets_[id] = w;
    return id;
}

std::optional<Wallet> Multisig::GetWallet(WalletId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = wallets_.find(id);
    if (it == wallets_.end()) return std::nullopt;
    return it->second;
}

size_t Multisig::WalletCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return wallets_.size();
}

bool Multisig::IsOwner(WalletId id, const std::string& who) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = wallets_.find(id);
    if (it == wallets_.end()) return false;
    const auto& v = it->second.owners;
    return std::find(v.begin(), v.end(), who) != v.end();
}

uint32_t Multisig::Threshold(WalletId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = wallets_.find(id);
    return it == wallets_.end() ? 0 : it->second.threshold;
}

std::vector<std::string> Multisig::Owners(WalletId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = wallets_.find(id);
    if (it == wallets_.end()) return {};
    return it->second.owners;
}

bool Multisig::AddOwner(WalletId id, const std::string& caller, const std::string& new_owner) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = wallets_.find(id);
    if (it == wallets_.end()) return false;
    if (!IsOwner(id, caller)) return false;
    if (new_owner.empty()) return false;
    auto& v = it->second.owners;
    if (std::find(v.begin(), v.end(), new_owner) != v.end()) return false;
    v.push_back(new_owner);
    return true;
}

bool Multisig::RemoveOwner(WalletId id, const std::string& caller, const std::string& owner) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = wallets_.find(id);
    if (it == wallets_.end()) return false;
    if (!IsOwner(id, caller)) return false;
    auto& v = it->second.owners;
    auto pos = std::find(v.begin(), v.end(), owner);
    if (pos == v.end()) return false;
    if (v.size() <= 1) return false;                     // cannot remove last owner
    if (it->second.threshold > v.size() - 1) return false; // threshold would exceed
    v.erase(pos);
    return true;
}

bool Multisig::ChangeThreshold(WalletId id, const std::string& caller, uint32_t new_threshold) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = wallets_.find(id);
    if (it == wallets_.end()) return false;
    if (!IsOwner(id, caller)) return false;
    if (new_threshold == 0 || new_threshold > it->second.owners.size()) return false;
    it->second.threshold = new_threshold;
    return true;
}

// ---- Funding ----
bool Multisig::Deposit(WalletId id, uint64_t amount) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = wallets_.find(id);
    if (it == wallets_.end() || amount == 0) return false;
    it->second.balance += amount;
    return true;
}

uint64_t Multisig::Balance(WalletId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = wallets_.find(id);
    return it == wallets_.end() ? 0 : it->second.balance;
}

// ---- Transactions ----
std::optional<TxId> Multisig::Propose(WalletId id,
                                      const std::string& proposer,
                                      const std::string& to,
                                      uint64_t amount,
                                      const std::string& memo) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = wallets_.find(id);
    if (it == wallets_.end()) return std::nullopt;
    if (!IsOwner(id, proposer)) return std::nullopt;
    if (to.empty() || amount == 0) return std::nullopt;
    if (amount > it->second.balance) return std::nullopt;

    TxId tx_id = next_tx_id_++;
    Transaction t;
    t.id = tx_id;
    t.wallet = id;
    t.proposer = proposer;
    t.to = to;
    t.amount = amount;
    t.memo = memo;
    t.nonce = it->second.nonce + 1;
    t.state = TxState::Pending;
    t.created_at = (int64_t)std::time(nullptr);
    t.approvals.insert(proposer);   // auto-approve by proposer
    txs_[tx_id] = t;
    wallet_txs_[id].push_back(tx_id);
    return tx_id;
}

bool Multisig::Approve(WalletId id, TxId tx_id, const std::string& approver) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = txs_.find(tx_id);
    if (it == txs_.end()) return false;
    if (it->second.wallet != id) return false;
    if (it->second.state != TxState::Pending) return false;
    if (!IsOwner(id, approver)) return false;
    if (it->second.approvals.count(approver)) return false;   // already approved
    it->second.approvals.insert(approver);
    return true;
}

bool Multisig::RevokeApproval(WalletId id, TxId tx_id, const std::string& approver) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = txs_.find(tx_id);
    if (it == txs_.end()) return false;
    if (it->second.wallet != id) return false;
    if (it->second.state != TxState::Pending) return false;
    if (it->second.proposer == approver) return false;   // proposer cannot revoke (must cancel)
    return it->second.approvals.erase(approver) > 0;
}

bool Multisig::Execute(WalletId id, TxId tx_id, const std::string& caller) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = txs_.find(tx_id);
    if (it == txs_.end()) return false;
    if (it->second.wallet != id) return false;
    if (it->second.state != TxState::Pending) return false;
    if (!IsOwner(id, caller)) return false;

    auto wit = wallets_.find(id);
    if (wit == wallets_.end()) return false;
    if (it->second.approvals.size() < wit->second.threshold) return false;
    if (it->second.amount > wit->second.balance) return false;

    wit->second.balance -= it->second.amount;
    wit->second.nonce += 1;
    it->second.state = TxState::Executed;
    it->second.executed_at = (int64_t)std::time(nullptr);
    return true;
}

bool Multisig::Cancel(WalletId id, TxId tx_id, const std::string& caller) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = txs_.find(tx_id);
    if (it == txs_.end()) return false;
    if (it->second.wallet != id) return false;
    if (it->second.state != TxState::Pending) return false;
    if (it->second.proposer != caller) return false;   // only proposer can cancel
    it->second.state = TxState::Cancelled;
    return true;
}

// ---- Queries ----
std::optional<Transaction> Multisig::GetTransaction(TxId tx_id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = txs_.find(tx_id);
    if (it == txs_.end()) return std::nullopt;
    return it->second;
}

std::vector<TxId> Multisig::PendingTxs(WalletId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<TxId> out;
    auto it = wallet_txs_.find(id);
    if (it == wallet_txs_.end()) return out;
    for (auto tx : it->second) {
        auto tit = txs_.find(tx);
        if (tit != txs_.end() && tit->second.state == TxState::Pending)
            out.push_back(tx);
    }
    return out;
}

size_t Multisig::ApprovalCount(TxId tx_id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = txs_.find(tx_id);
    return it == txs_.end() ? 0 : it->second.approvals.size();
}

bool Multisig::HasApproved(TxId tx_id, const std::string& who) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = txs_.find(tx_id);
    if (it == txs_.end()) return false;
    return it->second.approvals.count(who) > 0;
}

size_t Multisig::TransactionCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return txs_.size();
}

}