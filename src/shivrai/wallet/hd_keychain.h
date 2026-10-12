#ifndef SHIVRAI_WALLET_HD_KEYCHAIN_H
#define SHIVRAI_WALLET_HD_KEYCHAIN_H

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <optional>

namespace shivrai {

// Shivrai coin type per SLIP-44.
static constexpr uint32_t SHIVRAI_COIN_TYPE = 9999;

// BIP44 purpose
static constexpr uint32_t BIP44_PURPOSE = 44;
static constexpr uint32_t BIP44_HARDENED = 0x80000000u;

/** Extended public key (BIP32). */
struct ExtPubKey {
    std::array<uint8_t, 33> pubkey{};       // compressed secp256k1 pubkey
    std::array<uint8_t, 32> chaincode{};
    uint32_t depth{0};
    uint32_t parent_fingerprint{0};
    uint32_t child_number{0};

    std::string ToBase58() const;
    std::array<uint8_t, 78> Serialize() const;
    static std::optional<ExtPubKey> FromBase58(const std::string&);
};

/** Extended private key (BIP32). */
struct ExtKey {
    std::array<uint8_t, 32> key{};          // secp256k1 private key
    std::array<uint8_t, 32> chaincode{};
    uint32_t depth{0};
    uint32_t parent_fingerprint{0};
    uint32_t child_number{0};

    std::string ToBase58() const;
    std::array<uint8_t, 78> Serialize() const;
    static std::optional<ExtKey> FromBase58(const std::string&);

    ExtKey DeriveChild(uint32_t index) const;
    ExtPubKey Neuter() const;

    /** Path parser: "m/44'/9999'/0'/0/0" -> [2147483692, 2147483699, ...] */
    static std::vector<uint32_t> ParsePath(const std::string& path);
};

/** BIP32/39/44 HD keychain. */
class HDKeychain {
public:
    HDKeychain();

    /** Initialize from BIP39 seed (64 bytes). */
    bool InitFromSeed(const std::vector<uint8_t>& seed);

    /** Initialize from BIP39 mnemonic (uses PBKDF2-HMAC-SHA512). */
    bool InitFromMnemonic(const std::string& mnemonic, const std::string& passphrase = "");

    /** Get master extended private key. */
    const ExtKey& MasterKey() const { return master_; }

    /** Derive a key by BIP32 path from master. */
    std::optional<ExtKey> DerivePath(const std::string& path) const;

    /** Derive BIP44 key: m/44'/9999'/account'/change/index */
    std::optional<ExtKey> DeriveBIP44(uint32_t account, uint32_t change, uint32_t index) const;

    /** Derive BIP44 public key. */
    std::optional<ExtPubKey> DeriveBIP44Pub(uint32_t account, uint32_t change, uint32_t index) const;

    /** Fingerprint of master. */
    uint32_t MasterFingerprint() const;

    bool IsInitialized() const { return initialized_; }

private:
    ExtKey master_;
    bool initialized_{false};
};


/** Simple pubkey + hash160 bundle (no Bitcoin Core dependency). */
struct CPubKeyBytes {
    std::array<uint8_t, 33> bytes{};
    std::array<uint8_t, 20> h160{};
};

} // namespace shivrai

#endif
