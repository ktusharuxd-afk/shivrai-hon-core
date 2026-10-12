#include "shivrai/wallet/wallet.h"
#include "shivrai/wallet/bech32.h"
#include "shivrai/wallet/mnemonic.h"

#include <openssl/sha.h>
#include <openssl/ripemd.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <sstream>
#include <stdexcept>

namespace shivrai::wallet {

static int64_t now_seconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

ShivraiWallet::ShivraiWallet(std::string name)
    : name_(std::move(name)), last_activity_(now_seconds()) {}

ShivraiWallet::~ShivraiWallet() = default;

// ---------------- HD ----------------

bool ShivraiWallet::InitHD(const std::vector<uint8_t>& seed) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return hd_.InitFromSeed(seed);
}

bool ShivraiWallet::InitFromMnemonic(const std::string& mnemonic, const std::string& passphrase) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (!mnemonic::Validate(mnemonic)) return false;
    if (!hd_.InitFromMnemonic(mnemonic, passphrase)) return false;
    stored_mnemonic_ = mnemonic;
    return true;
}

// ---------------- Encryption ----------------

bool ShivraiWallet::EncryptWallet(const std::string& passphrase) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (encrypted_) return false;
    if (!keystore_.AddKey(hd_.MasterKey(), "master", passphrase)) return false;
    encrypted_ = true;
    locked_ = true;
    cached_passphrase_.clear();
    return true;
}

bool ShivraiWallet::Unlock(const std::string& passphrase, int timeout_seconds) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (!encrypted_) { locked_ = false; return true; }
    auto k = keystore_.GetKey(1, passphrase);
    if (!k) return false;
    cached_passphrase_ = passphrase;
    locked_ = false;
    autolock_seconds_ = timeout_seconds;
    last_activity_ = now_seconds();
    return true;
}

void ShivraiWallet::Lock() {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    locked_ = true;
    cached_passphrase_.clear();
}

bool ShivraiWallet::CheckAutoLock() {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (!encrypted_ || locked_) return false;
    int64_t elapsed = now_seconds() - last_activity_;
    if (elapsed >= autolock_seconds_) {
        locked_ = true;
        cached_passphrase_.clear();
        return true;
    }
    return false;
}

// ---------------- Addresses ----------------

std::string ShivraiWallet::AddressFromPubkey(const CPubKeyBytes& pub, bool bech32) const {
    if (bech32) {
        std::vector<uint8_t> prog(pub.h160.begin(), pub.h160.end());
        return bech32::EncodeSegwit(SHIVRAI_BECH32_HRP, 0, prog);
    }
    // Legacy: "H" + hex (simplified for standalone)
    static const char* H = "0123456789abcdef";
    std::string s = SHIVRAI_LEGACY_PREFIX;
    for (auto b : pub.h160) { s += H[b >> 4]; s += H[b & 15]; }
    return s;
}

std::optional<std::string> ShivraiWallet::NewAddress(const std::string& label, bool bech32) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (!hd_.IsInitialized()) return std::nullopt;
    auto ek = hd_.DeriveBIP44(0, 0, next_receive_idx_++);
    if (!ek) return std::nullopt;
    auto pub = ek->Neuter();
    CPubKeyBytes p;
    std::memcpy(p.bytes.data(), pub.pubkey.data(), 33);
    // h160 from pubkey
    uint8_t sha[32];
    SHA256(pub.pubkey.data(), 33, sha);
    uint8_t h160[20];
    RIPEMD160(sha, 32, h160);
    std::memcpy(p.h160.data(), h160, 20);
    std::string addr = AddressFromPubkey(p, bech32);
    if (!label.empty()) labels_[addr] = label;
    return addr;
}

std::optional<std::string> ShivraiWallet::ChangeAddress() {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (!hd_.IsInitialized()) return std::nullopt;
    auto ek = hd_.DeriveBIP44(0, 1, next_change_idx_++);
    if (!ek) return std::nullopt;
    auto pub = ek->Neuter();
    CPubKeyBytes p;
    std::memcpy(p.bytes.data(), pub.pubkey.data(), 33);
    uint8_t sha[32];
    SHA256(pub.pubkey.data(), 33, sha);
    uint8_t h160[20];
    RIPEMD160(sha, 32, h160);
    std::memcpy(p.h160.data(), h160, 20);
    return AddressFromPubkey(p, true);
}

bool ShivraiWallet::DecodeAddress(const std::string& addr, std::vector<uint8_t>& out) const {
    if (addr.size() > 4 && addr.compare(0, 4, "hon1") == 0) {
        int witver; std::vector<uint8_t> prog;
        if (!bech32::DecodeSegwit(SHIVRAI_BECH32_HRP, addr, witver, prog)) return false;
        out = prog;
        return true;
    }
    if (!addr.empty() && addr[0] == 'H') {
        // Simplified legacy: just return hex-decoded h160
        std::string hex = addr.substr(1);
        if (hex.size() != 40) return false;
        out.resize(20);
        for (int i = 0; i < 20; ++i) {
            auto hv = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                return -1;
            };
            int hi = hv(hex[2*i]), lo = hv(hex[2*i+1]);
            if (hi < 0 || lo < 0) return false;
            out[i] = (uint8_t)((hi << 4) | lo);
        }
        return true;
    }
    return false;
}

// ---------------- Mnemonic ----------------

std::string ShivraiWallet::DumpMnemonic(const std::string& passphrase) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (encrypted_ && locked_) {
        auto k = keystore_.GetKey(1, passphrase);
        if (!k) return "";
    }
    return stored_mnemonic_;
}

bool ShivraiWallet::ImportMnemonic(const std::string& mnemonic, const std::string& passphrase) {
    return InitFromMnemonic(mnemonic, passphrase);
}

// ---------------- Transactions ----------------

void ShivraiWallet::FundAddress(const std::string& address, uint64_t amount) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    balances_[address] += amount;
}

std::string ShivraiWallet::GenTxid() {
    std::ostringstream oss;
    oss << "tx" << (++txid_counter_) << "-" << now_seconds();
    return oss.str();
}

std::optional<std::string> ShivraiWallet::SendToAddress(const std::string& to,
                                                         uint64_t amount,
                                                         const std::string& memo) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    (void)memo;
    // Find a funding address with sufficient balance
    std::string from;
    for (auto& kv : balances_) {
        if (kv.second >= amount + SHIVRAI_DEFAULT_FEE) { from = kv.first; break; }
    }
    if (from.empty()) return std::nullopt;

    WalletTx tx;
    tx.txid = GenTxid();
    tx.from_address = from;
    tx.to_address = to;
    tx.amount = amount;
    tx.fee = SHIVRAI_DEFAULT_FEE;
    tx.timestamp = now_seconds();
    tx.confirmed = false;

    balances_[from] -= (amount + SHIVRAI_DEFAULT_FEE);
    balances_[to]   += amount;
    txs_[tx.txid] = tx;
    last_activity_ = now_seconds();
    return tx.txid;
}

bool ShivraiWallet::AddTx(const WalletTx& tx) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (txs_.count(tx.txid)) return false;
    txs_[tx.txid] = tx;
    return true;
}

WalletBalance ShivraiWallet::GetBalance() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    WalletBalance b;
    for (const auto& kv : balances_) b.confirmed += kv.second;
    return b;
}

std::vector<WalletTx> ShivraiWallet::ListTransactions() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<WalletTx> out;
    for (const auto& kv : txs_) out.push_back(kv.second);
    return out;
}

std::optional<WalletTx> ShivraiWallet::GetTransaction(const std::string& txid) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = txs_.find(txid);
    if (it == txs_.end()) return std::nullopt;
    return it->second;
}

// ---------------- Multi-asset ----------------

uint64_t ShivraiWallet::GetAssetBalance(const std::string& addr, const std::string& asset_id) const {
    return assets_.BalanceOf(addr, asset_id);
}

std::vector<AssetBalance> ShivraiWallet::AddressBalances(const std::string& addr) const {
    return assets_.BalancesOf(addr);
}

// ---------------- NFTs ----------------

bool ShivraiWallet::MintNft(const std::string& asset_id, uint32_t token_id,
                             const std::string& owner, const std::string& uri,
                             std::map<std::string,std::string> metadata) {
    NftRecord r;
    r.asset_id = asset_id;
    r.token_id = token_id;
    r.owner_address = owner;
    r.uri = uri;
    r.metadata = std::move(metadata);
    return assets_.MintNft(r);
}

bool ShivraiWallet::SendNft(const std::string& asset_id, uint32_t token_id,
                             const std::string& to, std::string& err) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto n = assets_.GetNft(asset_id, token_id);
    if (!n) { err = "NFT not found"; return false; }
    if (!assets_.TransferNft(asset_id, token_id, to)) { err = "transfer failed"; return false; }
    return true;
}

std::vector<NftRecord> ShivraiWallet::NftsOf(const std::string& addr) const {
    return assets_.NftsOf(addr);
}

// ---------------- Stats ----------------

size_t ShivraiWallet::TxCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return txs_.size();
}

size_t ShivraiWallet::KeyCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return keystore_.Size();
}

}  // namespace shivrai::wallet
