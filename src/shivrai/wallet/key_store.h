#ifndef SHIVRAI_WALLET_KEY_STORE_H
#define SHIVRAI_WALLET_KEY_STORE_H

#include "shivrai/wallet/argon2.h"
#include "shivrai/wallet/hd_keychain.h"

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace shivrai {

/** A record of one encrypted key blob on disk. */
struct EncryptedKeyRecord {
    uint32_t id;
    std::string label;
    std::vector<uint8_t> ciphertext;   // includes 12-byte IV prepended
    std::vector<uint8_t> tag;          // 16-byte GCM tag
};

/** AES-256-GCM + Argon2id-encrypted key store. */
class KeyStore {
public:
    KeyStore();

    /** Configure KDF parameters (defaults: Argon2id t=3 m=64MiB p=4). */
    void SetKDFParams(const Argon2Params& p) { kdf_params_ = p; }

    /** Encrypt `plaintext` under `passphrase`; returns IV||ct and tag. */
    bool Encrypt(const std::vector<uint8_t>& plaintext,
                 const std::string& passphrase,
                 std::vector<uint8_t>& iv_ct_out,
                 std::vector<uint8_t>& tag_out,
                 std::vector<uint8_t>& salt_out) const;

    /** Decrypt blob. Returns empty on failure. */
    std::optional<std::vector<uint8_t>> Decrypt(const std::vector<uint8_t>& iv_ct,
                                                const std::vector<uint8_t>& tag,
                                                const std::vector<uint8_t>& salt,
                                                const std::string& passphrase) const;

    /** Add an extended private key, encrypted under passphrase. */
    bool AddKey(const ExtKey& key, const std::string& label, const std::string& passphrase);

    /** Remove a key by id. */
    bool RemoveKey(uint32_t id);

    /** List key ids + labels. */
    std::vector<std::pair<uint32_t, std::string>> ListKeys() const;

    /** Decrypt a key by id. */
    std::optional<ExtKey> GetKey(uint32_t id, const std::string& passphrase) const;

    /** Change passphrase for all records. */
    bool ReencryptAll(const std::string& old_pass, const std::string& new_pass);

    /** Whether any records exist. */
    bool Empty() const { return records_.empty(); }
    size_t Size() const { return records_.size(); }

private:
    mutable std::recursive_mutex cs_;
    std::map<uint32_t, EncryptedKeyRecord> records_;
    std::map<uint32_t, std::vector<uint8_t>> salts_;
    uint32_t next_id_{1};
    Argon2Params kdf_params_;
};

} // namespace shivrai

#endif
