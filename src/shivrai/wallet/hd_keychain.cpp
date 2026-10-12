#include "shivrai/wallet/hd_keychain.h"
#include "shivrai/wallet/bech32.h"

#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <openssl/evp.h>

#include <secp256k1.h>

#include <cstring>
#include <sstream>
#include <stdexcept>

namespace shivrai {

static const char* B58_ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

static std::string Base58Encode(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> digits;
    for (uint8_t b : data) {
        int carry = b;
        for (size_t i = 0; i < digits.size(); ++i) {
            carry += 256 * digits[i];
            digits[i] = carry % 58;
            carry /= 58;
        }
        while (carry) { digits.push_back(carry % 58); carry /= 58; }
    }
    std::string result;
    for (uint8_t b : data) { if (b == 0) result += '1'; else break; }
    for (auto it = digits.rbegin(); it != digits.rend(); ++it) result += B58_ALPHABET[*it];
    return result;
}

static std::optional<std::vector<uint8_t>> Base58Decode(const std::string& s) {
    std::vector<uint8_t> result;
    for (char c : s) {
        const char* p = strchr(B58_ALPHABET, c);
        if (!p) return std::nullopt;
        int val = (int)(p - B58_ALPHABET);
        int carry = val;
        for (size_t i = 0; i < result.size(); ++i) {
            carry += 58 * result[i];
            result[i] = carry & 0xff;
            carry >>= 8;
        }
        while (carry) { result.push_back(carry & 0xff); carry >>= 8; }
    }
    size_t nzeros = 0;
    for (char c : s) { if (c == '1') ++nzeros; else break; }
    std::vector<uint8_t> out(nzeros, 0);
    out.insert(out.end(), result.rbegin(), result.rend());
    return out;
}

static std::array<uint8_t, 32> Sha256(const uint8_t* data, size_t len) {
    std::array<uint8_t, 32> out{};
    SHA256(data, len, out.data());
    return out;
}

static std::array<uint8_t, 32> DoubleSha256(const std::vector<uint8_t>& v) {
    auto h1 = Sha256(v.data(), v.size());
    return Sha256(h1.data(), h1.size());
}

static std::array<uint8_t, 32> HmacSha512(const std::vector<uint8_t>& key, const std::vector<uint8_t>& data) {
    std::array<uint8_t, 32> out{};
    unsigned int outlen = 0;
    HMAC(EVP_sha512(), key.data(), (int)key.size(), data.data(), data.size(),
         out.data(), &outlen);
    // Return only first 32 bytes for chaincode convenience; caller uses full 64 for BIP32.
    return out;
}

static void HmacSha512Full(const std::vector<uint8_t>& key, const std::vector<uint8_t>& data,
                           uint8_t out[64]) {
    unsigned int outlen = 0;
    HMAC(EVP_sha512(), key.data(), (int)key.size(), data.data(), data.size(), out, &outlen);
}

static std::array<uint8_t, 20> Hash160(const uint8_t* data, size_t len) {
    uint8_t sha[32];
    SHA256(data, len, sha);
    std::array<uint8_t, 20> out{};
    RIPEMD160(sha, 32, out.data());
    return out;
}

static secp256k1_context* GetCtx() {
    static secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    return ctx;
}

static std::array<uint8_t, 33> PubkeyCompress(const uint8_t priv[32]) {
    secp256k1_pubkey pk;
    if (!secp256k1_ec_pubkey_create(GetCtx(), &pk, priv)) throw std::runtime_error("pubkey_create failed");
    std::array<uint8_t, 33> out{};
    size_t outlen = 33;
    secp256k1_ec_pubkey_serialize(GetCtx(), out.data(), &outlen, &pk, SECP256K1_EC_COMPRESSED);
    return out;
}

// --- ExtPubKey ---

std::array<uint8_t, 78> ExtPubKey::Serialize() const {
    std::array<uint8_t, 78> out{};
    out[0] = 0x04; out[1] = 0x88; out[2] = 0xB2; out[3] = 0x1E; // xpub version
    out[4] = (uint8_t)(depth >> 24); out[5] = (uint8_t)(depth >> 16);
    out[6] = (uint8_t)(depth >> 8);  out[7] = (uint8_t)depth;
    out[8]  = (uint8_t)(parent_fingerprint >> 24);
    out[9]  = (uint8_t)(parent_fingerprint >> 16);
    out[10] = (uint8_t)(parent_fingerprint >> 8);
    out[11] = (uint8_t)parent_fingerprint;
    out[12] = (uint8_t)(child_number >> 24);
    out[13] = (uint8_t)(child_number >> 16);
    out[14] = (uint8_t)(child_number >> 8);
    out[15] = (uint8_t)child_number;
    memcpy(out.data() + 16, chaincode.data(), 32);
    memcpy(out.data() + 48, pubkey.data(), 33);
    return out;
}

std::string ExtPubKey::ToBase58() const {
    auto ser = Serialize();
    std::vector<uint8_t> data(ser.begin(), ser.end());
    auto chk = DoubleSha256(data);
    data.insert(data.end(), chk.begin(), chk.begin() + 4);
    return Base58Encode(data);
}

std::optional<ExtPubKey> ExtPubKey::FromBase58(const std::string& s) {
    auto dec = Base58Decode(s);
    if (!dec || dec->size() != 82) return std::nullopt;
    auto chk = DoubleSha256(std::vector<uint8_t>(dec->begin(), dec->end() - 4));
    if (memcmp(chk.data(), dec->data() + dec->size() - 4, 4) != 0) return std::nullopt;
    ExtPubKey k;
    k.depth = ((uint32_t)(*dec)[4] << 24) | ((uint32_t)(*dec)[5] << 16) |
              ((uint32_t)(*dec)[6] << 8) | (*dec)[7];
    k.parent_fingerprint = ((uint32_t)(*dec)[8] << 24) | ((uint32_t)(*dec)[9] << 16) |
                           ((uint32_t)(*dec)[10] << 8) | (*dec)[11];
    k.child_number = ((uint32_t)(*dec)[12] << 24) | ((uint32_t)(*dec)[13] << 16) |
                     ((uint32_t)(*dec)[14] << 8) | (*dec)[15];
    memcpy(k.chaincode.data(), dec->data() + 16, 32);
    memcpy(k.pubkey.data(), dec->data() + 48, 33);
    return k;
}

// --- ExtKey ---

std::array<uint8_t, 78> ExtKey::Serialize() const {
    std::array<uint8_t, 78> out{};
    out[0] = 0x04; out[1] = 0x88; out[2] = 0xAD; out[3] = 0xE4; // xprv version
    out[4] = (uint8_t)(depth >> 24); out[5] = (uint8_t)(depth >> 16);
    out[6] = (uint8_t)(depth >> 8);  out[7] = (uint8_t)depth;
    out[8]  = (uint8_t)(parent_fingerprint >> 24);
    out[9]  = (uint8_t)(parent_fingerprint >> 16);
    out[10] = (uint8_t)(parent_fingerprint >> 8);
    out[11] = (uint8_t)parent_fingerprint;
    out[12] = (uint8_t)(child_number >> 24);
    out[13] = (uint8_t)(child_number >> 16);
    out[14] = (uint8_t)(child_number >> 8);
    out[15] = (uint8_t)child_number;
    memcpy(out.data() + 16, chaincode.data(), 32);
    out[45] = 0x00;
    memcpy(out.data() + 46, key.data(), 32);
    return out;
}

std::string ExtKey::ToBase58() const {
    auto ser = Serialize();
    std::vector<uint8_t> data(ser.begin(), ser.end());
    auto chk = DoubleSha256(data);
    data.insert(data.end(), chk.begin(), chk.begin() + 4);
    return Base58Encode(data);
}

std::optional<ExtKey> ExtKey::FromBase58(const std::string& s) {
    auto dec = Base58Decode(s);
    if (!dec || dec->size() != 82) return std::nullopt;
    auto chk = DoubleSha256(std::vector<uint8_t>(dec->begin(), dec->end() - 4));
    if (memcmp(chk.data(), dec->data() + dec->size() - 4, 4) != 0) return std::nullopt;
    ExtKey k;
    k.depth = ((uint32_t)(*dec)[4] << 24) | ((uint32_t)(*dec)[5] << 16) |
              ((uint32_t)(*dec)[6] << 8) | (*dec)[7];
    k.parent_fingerprint = ((uint32_t)(*dec)[8] << 24) | ((uint32_t)(*dec)[9] << 16) |
                           ((uint32_t)(*dec)[10] << 8) | (*dec)[11];
    k.child_number = ((uint32_t)(*dec)[12] << 24) | ((uint32_t)(*dec)[13] << 16) |
                     ((uint32_t)(*dec)[14] << 8) | (*dec)[15];
    memcpy(k.chaincode.data(), dec->data() + 16, 32);
    if ((*dec)[45] != 0x00) return std::nullopt;
    memcpy(k.key.data(), dec->data() + 46, 32);
    return k;
}

ExtKey ExtKey::DeriveChild(uint32_t index) const {
    std::vector<uint8_t> data;
    if (index & BIP44_HARDENED) {
        data.push_back(0x00);
        data.insert(data.end(), key.begin(), key.end());
    } else {
        auto pub = PubkeyCompress(key.data());
        data.insert(data.end(), pub.begin(), pub.end());
    }
    data.push_back((uint8_t)(index >> 24));
    data.push_back((uint8_t)(index >> 16));
    data.push_back((uint8_t)(index >> 8));
    data.push_back((uint8_t)index);

    uint8_t I[64];
    std::vector<uint8_t> cc(chaincode.begin(), chaincode.end());
    HmacSha512Full(cc, data, I);

    ExtKey child;
    memcpy(child.key.data(), I, 32);
    memcpy(child.chaincode.data(), I + 32, 32);
    child.depth = depth + 1;
    {
        auto parent_pub = PubkeyCompress(key.data());
        auto h = Hash160(parent_pub.data(), 33);
        child.parent_fingerprint = ((uint32_t)h[0] << 24) | ((uint32_t)h[1] << 16) |
                                   ((uint32_t)h[2] << 8) | h[3];
    }
    child.child_number = index;

    // child.key = (IL + parent.key) mod n
    secp256k1_context* ctx = GetCtx();
    if (!secp256k1_ec_seckey_tweak_add(ctx, child.key.data(), key.data())) {
        throw std::runtime_error("BIP32 derivation out of range");
    }
    return child;
}

ExtPubKey ExtKey::Neuter() const {
    ExtPubKey pk;
    auto pub = PubkeyCompress(key.data());
    memcpy(pk.pubkey.data(), pub.data(), 33);
    memcpy(pk.chaincode.data(), chaincode.data(), 32);
    pk.depth = depth;
    pk.parent_fingerprint = parent_fingerprint;
    pk.child_number = child_number;
    return pk;
}

uint32_t HDKeychain::MasterFingerprint() const {
    auto pub = PubkeyCompress(master_.key.data());
    auto h = Hash160(pub.data(), 33);
    return ((uint32_t)h[0] << 24) | ((uint32_t)h[1] << 16) | ((uint32_t)h[2] << 8) | h[3];
}

std::vector<uint32_t> ExtKey::ParsePath(const std::string& path) {
    std::vector<uint32_t> result;
    if (path.empty() || path == "m" || path == "M") return result;
    std::string p = path;
    if (p[0] == 'm' || p[0] == 'M') {
        if (p.size() > 1 && p[1] == '/') p = p.substr(2);
        else p = p.substr(1);
    }
    std::stringstream ss(p);
    std::string part;
    while (std::getline(ss, part, '/')) {
        if (part.empty()) continue;
        bool hardened = false;
        if (!part.empty() && (part.back() == '\'' || part.back() == 'h' || part.back() == 'H')) {
            hardened = true;
            part.pop_back();
        }
        uint32_t idx = (uint32_t)std::stoul(part);
        if (hardened) idx |= BIP44_HARDENED;
        result.push_back(idx);
    }
    return result;
}

// --- HDKeychain ---

HDKeychain::HDKeychain() = default;

bool HDKeychain::InitFromSeed(const std::vector<uint8_t>& seed) {
    if (seed.size() < 16) return false;
    uint8_t I[64];
    std::vector<uint8_t> key = {'B','i','t','c','o','i','n',' ','s','e','e','d'};
    HmacSha512Full(key, seed, I);
    memcpy(master_.key.data(), I, 32);
    memcpy(master_.chaincode.data(), I + 32, 32);
    master_.depth = 0;
    master_.parent_fingerprint = 0;
    master_.child_number = 0;
    initialized_ = true;
    return true;
}

bool HDKeychain::InitFromMnemonic(const std::string& mnemonic, const std::string& passphrase) {
    // BIP39 seed = PBKDF2-HMAC-SHA512(mnemonic, "mnemonic" + passphrase, 2048, 64)
    std::string salt = "mnemonic" + passphrase;
    uint8_t seed[64];
    if (!PKCS5_PBKDF2_HMAC(mnemonic.c_str(), (int)mnemonic.size(),
                           (const unsigned char*)salt.c_str(), (int)salt.size(),
                           2048, EVP_sha512(), 64, seed)) {
        return false;
    }
    std::vector<uint8_t> s(seed, seed + 64);
    return InitFromSeed(s);
}

std::optional<ExtKey> HDKeychain::DerivePath(const std::string& path) const {
    if (!initialized_) return std::nullopt;
    auto indices = ExtKey::ParsePath(path);
    ExtKey k = master_;
    for (uint32_t idx : indices) {
        k = k.DeriveChild(idx);
    }
    return k;
}

std::optional<ExtKey> HDKeychain::DeriveBIP44(uint32_t account, uint32_t change, uint32_t index) const {
    if (!initialized_) return std::nullopt;
    if (change > 1) return std::nullopt;
    if (index >= BIP44_HARDENED || account >= BIP44_HARDENED) return std::nullopt;

    ExtKey k = master_;
    k = k.DeriveChild(BIP44_PURPOSE | BIP44_HARDENED);
    k = k.DeriveChild(SHIVRAI_COIN_TYPE | BIP44_HARDENED);
    k = k.DeriveChild(account | BIP44_HARDENED);
    k = k.DeriveChild(change);      // not hardened
    k = k.DeriveChild(index);       // not hardened
    return k;
}

std::optional<ExtPubKey> HDKeychain::DeriveBIP44Pub(uint32_t account, uint32_t change, uint32_t index) const {
    auto ek = DeriveBIP44(account, change, index);
    if (!ek) return std::nullopt;
    return ek->Neuter();
}

} // namespace shivrai
