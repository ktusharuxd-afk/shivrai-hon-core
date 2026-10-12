#ifndef SHIVRAI_HON_WALLET_H
#define SHIVRAI_HON_WALLET_H

#include "shivrai/wallet/asset_registry.h"
#include "shivrai/wallet/hd_keychain.h"
#include "shivrai/wallet/key_store.h"

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace shivrai::wallet {

// Default constants
static constexpr uint64_t SHIVRAI_DEFAULT_FEE = 1000;
static constexpr int SHIVRAI_DEFAULT_AUTOLOCK_SECONDS = 900;
static constexpr const char* SHIVRAI_BECH32_HRP = "hon";
static constexpr const char* SHIVRAI_LEGACY_PREFIX = "H";

struct WalletBalance {
    uint64_t confirmed{0};
    uint64_t unconfirmed{0};
    uint64_t total() const { return confirmed + unconfirmed; }
};

struct WalletTx {
    std::string txid;
    std::string from_address;
    std::string to_address;
    uint64_t amount{0};
    uint64_t fee{0};
    int64_t timestamp{0};
    bool confirmed{false};
};

/** Standalone Shivrai wallet (no Bitcoin Core dependency). */
class ShivraiWallet {
public:
    explicit ShivraiWallet(std::string name);
    ~ShivraiWallet();

    ShivraiWallet(const ShivraiWallet&) = delete;
    ShivraiWallet& operator=(const ShivraiWallet&) = delete;

    const std::string& name() const { return name_; }

    // ----- HD / Keys -----
    bool InitHD(const std::vector<uint8_t>& seed);
    bool InitFromMnemonic(const std::string& mnemonic, const std::string& passphrase = "");
    HDKeychain& hd() { return hd_; }
    const HDKeychain& hd() const { return hd_; }
    KeyStore& keystore() { return keystore_; }
    const KeyStore& keystore() const { return keystore_; }

    // ----- Encryption / Auto-lock -----
    bool EncryptWallet(const std::string& passphrase);
    bool IsEncrypted() const { return encrypted_; }
    bool IsLocked() const { return locked_; }
    bool Unlock(const std::string& passphrase, int timeout_seconds = SHIVRAI_DEFAULT_AUTOLOCK_SECONDS);
    void Lock();
    int AutoLockSeconds() const { return autolock_seconds_; }
    void SetAutoLockSeconds(int s) { autolock_seconds_ = s; }
    bool CheckAutoLock();  // Returns true if auto-locked now

    // ----- Addresses -----
    std::optional<std::string> NewAddress(const std::string& label = "", bool bech32 = true);
    std::optional<std::string> ChangeAddress();
    bool DecodeAddress(const std::string& addr, std::vector<uint8_t>& script_out) const;

    // ----- Mnemonic -----
    std::string DumpMnemonic(const std::string& passphrase = "");
    bool ImportMnemonic(const std::string& mnemonic, const std::string& passphrase = "");

    // ----- Transactions -----
    WalletBalance GetBalance() const;
    std::vector<WalletTx> ListTransactions() const;
    std::optional<WalletTx> GetTransaction(const std::string& txid) const;
    std::optional<std::string> SendToAddress(const std::string& to,
                                              uint64_t amount,
                                              const std::string& memo = "");
    bool AddTx(const WalletTx& tx);
    void FundAddress(const std::string& address, uint64_t amount);

    // ----- Multi-asset -----
    AssetRegistry& assets() { return assets_; }
    const AssetRegistry& assets() const { return assets_; }
    uint64_t GetAssetBalance(const std::string& addr, const std::string& asset_id) const;
    std::vector<AssetBalance> AddressBalances(const std::string& addr) const;

    // ----- NFTs -----
    bool MintNft(const std::string& asset_id, uint32_t token_id,
                 const std::string& owner, const std::string& uri,
                 std::map<std::string,std::string> metadata = {});
    bool SendNft(const std::string& asset_id, uint32_t token_id,
                 const std::string& to, std::string& err);
    std::vector<NftRecord> NftsOf(const std::string& addr) const;

    // ----- Stats -----
    size_t TxCount() const;
    size_t KeyCount() const;

private:
    std::string AddressFromPubkey(const CPubKeyBytes& pub, bool bech32) const;
    std::string GenTxid();

    mutable std::recursive_mutex cs_;
    std::string name_;

    HDKeychain hd_;
    KeyStore keystore_;
    AssetRegistry assets_;

    bool encrypted_{false};
    bool locked_{false};
    std::string cached_passphrase_;
    std::string stored_mnemonic_;
    int autolock_seconds_{SHIVRAI_DEFAULT_AUTOLOCK_SECONDS};
    int64_t last_activity_{0};

    uint32_t next_receive_idx_{0};
    uint32_t next_change_idx_{0};

    std::map<std::string, uint64_t> balances_;    // address -> satoshis
    std::map<std::string, WalletTx> txs_;         // txid -> tx
    std::map<std::string, std::string> labels_;   // address -> label
    uint64_t txid_counter_{0};
};

}  // namespace shivrai::wallet

#endif
