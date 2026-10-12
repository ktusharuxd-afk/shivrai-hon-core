#ifndef SHIVRAI_WALLET_MNEMONIC_H
#define SHIVRAI_WALLET_MNEMONIC_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace shivrai {
namespace mnemonic {

/** Generate 128-bit (12 words) or 256-bit (24 words) BIP39 mnemonic. */
std::string Generate(int strength_bits = 128);

/** Validate mnemonic (checksum + wordlist). */
bool Validate(const std::string& mnemonic);

/** Convert mnemonic to 64-byte BIP39 seed. */
std::vector<uint8_t> ToSeed(const std::string& mnemonic, const std::string& passphrase = "");

/** Get the 2048-word list. */
const std::vector<std::string>& WordList();

/** Split on whitespace. */
std::vector<std::string> Split(const std::string&);

} // namespace mnemonic
} // namespace shivrai

#endif
