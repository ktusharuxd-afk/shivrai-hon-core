#ifndef SHIVRAI_WALLET_BECH32_H
#define SHIVRAI_WALLET_BECH32_H

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace shivrai {
namespace bech32 {

std::string Encode(const std::string& hrp, const std::vector<uint8_t>& data);
std::pair<std::string, std::vector<uint8_t>> Decode(const std::string& str);
std::vector<uint8_t> ConvertBits(const std::vector<uint8_t>& data, int frombits, int tobits, bool pad);

/** Encode a witness (segwit v0/v1+) address. */
std::string EncodeSegwit(const std::string& hrp, int witver, const std::vector<uint8_t>& prog);
bool DecodeSegwit(const std::string& hrp, const std::string& addr, int& witver, std::vector<uint8_t>& prog);

} // namespace bech32
} // namespace shivrai

#endif
