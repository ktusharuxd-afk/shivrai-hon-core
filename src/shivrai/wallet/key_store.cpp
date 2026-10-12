#include "shivrai/wallet/key_store.h"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <cstring>

namespace shivrai {

KeyStore::KeyStore() = default;

bool KeyStore::Encrypt(const std::vector<uint8_t>& plaintext,
                       const std::string& passphrase,
                       std::vector<uint8_t>& iv_ct_out,
                       std::vector<uint8_t>& tag_out,
                       std::vector<uint8_t>& salt_out) const {
    salt_out.resize(16);
    if (RAND_bytes(salt_out.data(), 16) != 1) return false;

    std::vector<uint8_t> pw(passphrase.begin(), passphrase.end());
    auto dk = Argon2id(pw, salt_out, kdf_params_);
    if (dk.size() != 32) return false;

    std::vector<uint8_t> iv(12);
    if (RAND_bytes(iv.data(), 12) != 1) return false;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;

    bool ok = false;
    do {
        if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr)) break;
        if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr)) break;
        if (1 != EVP_EncryptInit_ex(ctx, nullptr, nullptr, dk.data(), iv.data())) break;

        std::vector<uint8_t> ct(plaintext.size() + 16);
        int outlen = 0;
        if (1 != EVP_EncryptUpdate(ctx, ct.data(), &outlen, plaintext.data(), (int)plaintext.size())) break;
        int total = outlen;
        if (1 != EVP_EncryptFinal_ex(ctx, ct.data() + total, &outlen)) break;
        total += outlen;
        ct.resize(total);

        tag_out.resize(16);
        if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag_out.data())) break;

        iv_ct_out = iv;
        iv_ct_out.insert(iv_ct_out.end(), ct.begin(), ct.end());
        ok = true;
    } while (0);

    EVP_CIPHER_CTX_free(ctx);
    return ok;
}

std::optional<std::vector<uint8_t>> KeyStore::Decrypt(const std::vector<uint8_t>& iv_ct,
                                                     const std::vector<uint8_t>& tag,
                                                     const std::vector<uint8_t>& salt,
                                                     const std::string& passphrase) const {
    if (iv_ct.size() < 12 || tag.size() != 16 || salt.size() != 16) return std::nullopt;

    std::vector<uint8_t> pw(passphrase.begin(), passphrase.end());
    auto dk = Argon2id(pw, salt, kdf_params_);
    if (dk.size() != 32) return std::nullopt;

    std::vector<uint8_t> iv(iv_ct.begin(), iv_ct.begin() + 12);
    std::vector<uint8_t> ct(iv_ct.begin() + 12, iv_ct.end());

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return std::nullopt;

    std::optional<std::vector<uint8_t>> result;
    do {
        if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr)) break;
        if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr)) break;
        if (1 != EVP_DecryptInit_ex(ctx, nullptr, nullptr, dk.data(), iv.data())) break;

        std::vector<uint8_t> pt(ct.size());
        int outlen = 0;
        if (1 != EVP_DecryptUpdate(ctx, pt.data(), &outlen, ct.data(), (int)ct.size())) break;
        int total = outlen;

        std::vector<uint8_t> tag_copy = tag;
        if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag_copy.data())) break;

        int ret = EVP_DecryptFinal_ex(ctx, pt.data() + total, &outlen);
        if (ret <= 0) break; // auth failure
        total += outlen;
        pt.resize(total);
        result = std::move(pt);
    } while (0);

    EVP_CIPHER_CTX_free(ctx);
    return result;
}

bool KeyStore::AddKey(const ExtKey& key, const std::string& label, const std::string& passphrase) {
    std::lock_guard<std::recursive_mutex> lock(cs_);
    auto ser = key.Serialize();
    std::vector<uint8_t> plain(ser.begin(), ser.end());

    EncryptedKeyRecord rec;
    rec.id = next_id_++;
    rec.label = label;
    std::vector<uint8_t> salt;
    if (!Encrypt(plain, passphrase, rec.ciphertext, rec.tag, salt)) return false;
    records_[rec.id] = std::move(rec);
    salts_[rec.id] = std::move(salt);
    return true;
}

bool KeyStore::RemoveKey(uint32_t id) {
    std::lock_guard<std::recursive_mutex> lock(cs_);
    auto it = records_.find(id);
    if (it == records_.end()) return false;
    records_.erase(it);
    salts_.erase(id);
    return true;
}

std::vector<std::pair<uint32_t, std::string>> KeyStore::ListKeys() const {
    std::lock_guard<std::recursive_mutex> lock(cs_);
    std::vector<std::pair<uint32_t, std::string>> out;
    for (const auto& kv : records_) out.emplace_back(kv.first, kv.second.label);
    return out;
}

std::optional<ExtKey> KeyStore::GetKey(uint32_t id, const std::string& passphrase) const {
    std::lock_guard<std::recursive_mutex> lock(cs_);
    auto it = records_.find(id);
    if (it == records_.end()) return std::nullopt;
    auto sit = salts_.find(id);
    if (sit == salts_.end()) return std::nullopt;
    auto pt = Decrypt(it->second.ciphertext, it->second.tag, sit->second, passphrase);
    if (!pt || pt->size() != 78) return std::nullopt;
    ExtKey k;
    // Parse from 78-byte serialization (skip 4-byte version, then 4-byte depth/... standard).
    std::vector<uint8_t> data(pt->begin(), pt->end());
    uint8_t chk[32]; // unused
    (void)chk;
    // Serialize() layout: [0..3]=version, [4..7]=depth, [8..11]=parent_fp,
    // [12..15]=child_number, [16..47]=chaincode, [45]=0x00, [46..77]=key.
    k.depth = ((uint32_t)data[4] << 24) | ((uint32_t)data[5] << 16) |
              ((uint32_t)data[6] << 8) | data[7];
    k.parent_fingerprint = ((uint32_t)data[8] << 24) | ((uint32_t)data[9] << 16) |
                           ((uint32_t)data[10] << 8) | data[11];
    k.child_number = ((uint32_t)data[12] << 24) | ((uint32_t)data[13] << 16) |
                     ((uint32_t)data[14] << 8) | data[15];
    memcpy(k.chaincode.data(), data.data() + 16, 32);
    memcpy(k.key.data(), data.data() + 46, 32);
    return k;
}

bool KeyStore::ReencryptAll(const std::string& old_pass, const std::string& new_pass) {
    std::lock_guard<std::recursive_mutex> lock(cs_);
    std::vector<uint32_t> ids;
    for (const auto& kv : records_) ids.push_back(kv.first);
    for (uint32_t id : ids) {
        auto k = GetKey(id, old_pass);
        if (!k) return false;
        std::string label = records_[id].label;
        RemoveKey(id);
        if (!AddKey(*k, label, new_pass)) return false;
    }
    return true;
}

} // namespace shivrai
