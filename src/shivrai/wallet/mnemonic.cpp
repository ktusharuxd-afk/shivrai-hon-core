#include "shivrai/wallet/mnemonic.h"
#include "shivrai/wallet/wordlist.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <sstream>
#include <unordered_map>
#include <cstring>

namespace shivrai { namespace mnemonic {

namespace {
std::vector<std::string> ParseWordlist() {
    std::vector<std::string> out;
    std::stringstream ss(WORDLIST_TEXT);
    std::string w;
    while (ss >> w) out.push_back(w);
    return out;
}
}

const std::vector<std::string>& WordList() {
    static std::vector<std::string> list = ParseWordlist();
    return list;
}

std::vector<std::string> Split(const std::string& s) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string w;
    while (ss >> w) out.push_back(w);
    return out;
}

std::string Generate(int strength_bits) {
    if (strength_bits != 128 && strength_bits != 256) strength_bits = 128;
    const int entropy_bytes = strength_bits / 8;
    std::vector<uint8_t> entropy(entropy_bytes);
    if (RAND_bytes(entropy.data(), entropy_bytes) != 1) return "";

    const int cs_bits = strength_bits / 32;
    uint8_t cs_byte = 0;
    {
        uint8_t h[32];
        SHA256(entropy.data(), entropy.size(), h);
        cs_byte = h[0]; // top cs_bits are enough for cs<=8
    }

    // Build the bit stream.
    std::vector<uint8_t> bits;
    bits.reserve(strength_bits + cs_bits);
    for (uint8_t b : entropy) for (int i = 7; i >= 0; --i) bits.push_back((b >> i) & 1);
    for (int i = 0; i < cs_bits; ++i) bits.push_back((cs_byte >> (7 - i)) & 1);

    const auto& list = WordList();
    std::string out;
    for (size_t i = 0; i + 11 <= bits.size(); i += 11) {
        int idx = 0;
        for (int k = 0; k < 11; ++k) idx = (idx << 1) | bits[i + k];
        if (!out.empty()) out += ' ';
        out += list[idx];
    }
    return out;
}

bool Validate(const std::string& mnemonic) {
    auto words = Split(mnemonic);
    if (words.size() != 12 && words.size() != 24) return false;
    const auto& list = WordList();
    std::unordered_map<std::string, int> idx;
    for (size_t i = 0; i < list.size(); ++i) idx[list[i]] = (int)i;

    std::vector<uint8_t> bits;
    bits.reserve(words.size() * 11);
    for (const auto& w : words) {
        auto it = idx.find(w);
        if (it == idx.end()) return false;
        int v = it->second;
        for (int k = 10; k >= 0; --k) bits.push_back((v >> k) & 1);
    }
    const size_t total = bits.size();
    const size_t cs_bits = total / 33;
    const size_t ent_bits = total - cs_bits;

    std::vector<uint8_t> entropy((ent_bits + 7) / 8, 0);
    for (size_t i = 0; i < ent_bits; ++i) {
        if (bits[i]) entropy[i / 8] |= (1 << (7 - (i % 8)));
    }
    uint8_t h[32];
    SHA256(entropy.data(), entropy.size(), h);
    for (size_t i = 0; i < cs_bits; ++i) {
        uint8_t expected = (h[i / 8] >> (7 - (i % 8))) & 1;
        if (bits[ent_bits + i] != expected) return false;
    }
    return true;
}

std::vector<uint8_t> ToSeed(const std::string& mnemonic, const std::string& passphrase) {
    std::string salt = "mnemonic" + passphrase;
    std::vector<uint8_t> seed(64);
    PKCS5_PBKDF2_HMAC(mnemonic.c_str(), (int)mnemonic.size(),
                      (const unsigned char*)salt.c_str(), (int)salt.size(),
                      2048, EVP_sha512(), 64, seed.data());
    return seed;
}

}} // namespace
