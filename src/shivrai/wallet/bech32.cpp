#include "shivrai/wallet/bech32.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace shivrai { namespace bech32 {

static const char* CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
static const int8_t CHARSET_REV[128] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    15,-1,10,17,21,20,26,30, 7, 5,-1,-1,-1,-1,-1,-1,
    -1,29,-1,24,13,25, 9, 8,23,-1,18,22,31,27,19,-1,
    1, 0, 3,16,11,28,12,14, 6, 4, 2,-1,-1,-1,-1,-1,
    -1,29,-1,24,13,25, 9, 8,23,-1,18,22,31,27,19,-1,
    1, 0, 3,16,11,28,12,14, 6, 4, 2,-1,-1,-1,-1,-1
};

static uint32_t Polymod(const std::vector<uint8_t>& v) {
    static const uint32_t GEN[5] = {0x3b6a57b2,0x26508e6d,0x1ea119fa,0x3d4233dd,0x2a1462b3};
    uint32_t chk = 1;
    for (uint8_t x : v) {
        uint8_t top = chk >> 25;
        chk = ((chk & 0x1ffffff) << 5) ^ x;
        for (int i = 0; i < 5; ++i) if ((top >> i) & 1) chk ^= GEN[i];
    }
    return chk;
}

static std::vector<uint8_t> HrpExpand(const std::string& hrp) {
    std::vector<uint8_t> out;
    for (char c : hrp) out.push_back((uint8_t)(c >> 5));
    out.push_back(0);
    for (char c : hrp) out.push_back((uint8_t)(c & 31));
    return out;
}

static uint32_t CreateChecksum(const std::string& hrp, const std::vector<uint8_t>& data, bool bech32m) {
    auto values = HrpExpand(hrp);
    values.insert(values.end(), data.begin(), data.end());
    values.insert(values.end(), 6, 0);
    uint32_t mod = Polymod(values) ^ (bech32m ? 0x2bc830a3u : 1u);
    return mod;
}

static bool VerifyChecksum(const std::string& hrp, const std::vector<uint8_t>& data, bool& bech32m) {
    auto values = HrpExpand(hrp);
    values.insert(values.end(), data.begin(), data.end());
    uint32_t p = Polymod(values);
    if (p == 1) { bech32m = false; return true; }
    if (p == 0x2bc830a3u) { bech32m = true; return true; }
    return false;
}


std::string Encode(const std::string& hrp, const std::vector<uint8_t>& data, bool bech32m);

std::string EncodeImpl(const std::string& hrp, const std::vector<uint8_t>& data, bool bech32m) {
    uint32_t mod = CreateChecksum(hrp, data, bech32m);
    std::vector<uint8_t> combined = data;
    for (int i = 0; i < 6; ++i) combined.push_back((uint8_t)((mod >> (5 * (5 - i))) & 31));
    std::string result = hrp + "1";
    for (uint8_t v : combined) result += CHARSET[v];
    return result;
}

std::string Encode(const std::string& hrp, const std::vector<uint8_t>& data) {
    return EncodeImpl(hrp, data, false);
}

std::pair<std::string, std::vector<uint8_t>> Decode(const std::string& str) {
    bool has_lower = false, has_upper = false;
    for (char c : str) {
        if (c < 33 || c > 126) throw std::runtime_error("bech32: invalid char");
        if (c >= 'a' && c <= 'z') has_lower = true;
        if (c >= 'A' && c <= 'Z') has_upper = true;
    }
    if (has_lower && has_upper) throw std::runtime_error("bech32: mixed case");
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);

    size_t pos = s.rfind('1');
    if (pos == std::string::npos || pos == 0 || pos + 7 > s.size())
        throw std::runtime_error("bech32: bad separator");

    std::string hrp = s.substr(0, pos);
    std::vector<uint8_t> data;
    for (size_t i = pos + 1; i < s.size(); ++i) {
        int8_t v = CHARSET_REV[(uint8_t)s[i]];
        if (v < 0) throw std::runtime_error("bech32: bad data char");
        data.push_back((uint8_t)v);
    }
    bool m;
    if (!VerifyChecksum(hrp, data, m)) throw std::runtime_error("bech32: bad checksum");
    data.resize(data.size() - 6);
    return {hrp, data};
}

std::vector<uint8_t> ConvertBits(const std::vector<uint8_t>& data, int frombits, int tobits, bool pad) {
    int acc = 0, bits = 0;
    std::vector<uint8_t> ret;
    int maxv = (1 << tobits) - 1;
    int max_acc = (1 << (frombits + tobits - 1)) - 1;
    for (uint8_t value : data) {
        if ((value >> frombits) != 0) throw std::runtime_error("convertbits: invalid");
        acc = ((acc << frombits) | value) & max_acc;
        bits += frombits;
        while (bits >= tobits) { bits -= tobits; ret.push_back((acc >> bits) & maxv); }
    }
    if (pad) { if (bits) ret.push_back((acc << (tobits - bits)) & maxv); }
    else if (bits >= frombits || ((acc << (tobits - bits)) & maxv))
        throw std::runtime_error("convertbits: bad padding");
    return ret;
}

std::string EncodeSegwit(const std::string& hrp, int witver, const std::vector<uint8_t>& prog) {
    std::vector<uint8_t> data;
    data.push_back((uint8_t)witver);
    auto conv = ConvertBits(prog, 8, 5, true);
    data.insert(data.end(), conv.begin(), conv.end());
    bool bech32m = (witver != 0);
    return EncodeImpl(hrp, data, bech32m);
}

bool DecodeSegwit(const std::string& hrp, const std::string& addr, int& witver, std::vector<uint8_t>& prog) {
    try {
        auto [h, data] = Decode(addr);
        if (h != hrp || data.empty()) return false;
        witver = data[0];
        auto conv = ConvertBits(std::vector<uint8_t>(data.begin() + 1, data.end()), 5, 8, false);
        prog = conv;
        return true;
    } catch (...) { return false; }
}

}} // namespace
