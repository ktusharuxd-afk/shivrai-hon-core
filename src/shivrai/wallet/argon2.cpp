#include "shivrai/wallet/argon2.h"
#include "shivrai/wallet/blake2b.h"

#include <cstring>
#include <stdexcept>
#include <algorithm>

namespace shivrai {

namespace {

constexpr uint32_t ARGON2_BLOCK_SIZE = 1024; // bytes
constexpr uint32_t ARGON2_QWORDS_IN_BLOCK = 128;
constexpr uint32_t ARGON2_SYNC_POINTS = 4;
constexpr uint32_t ARGON2_VERSION = 0x13;

struct Block { uint64_t v[ARGON2_QWORDS_IN_BLOCK]; };

inline uint64_t ROTR64(uint64_t x, uint64_t n) { return (x >> n) | (x << (64 - n)); }

inline uint64_t Fb(uint64_t x, uint64_t y) { return x + y + 2ULL * (uint32_t)x * (uint32_t)y; }

inline void G(Block& b, int a, int bb, int c, int d) {
    uint64_t x = b.v[a], y = b.v[bb], z = b.v[c], w = b.v[d];
    x = Fb(x, y); w = ROTR64(w ^ x, 32);
    z = Fb(z, w); y = ROTR64(y ^ z, 24);
    x = Fb(x, y); w = ROTR64(w ^ x, 16);
    z = Fb(z, w); y = ROTR64(y ^ z, 63);
    b.v[a] = x; b.v[bb] = y; b.v[c] = z; b.v[d] = w;
}

inline void Round(Block& b) {
    for (int i = 0; i < 8; ++i) {
        G(b, 16*i,     16*i + 1, 16*i + 2, 16*i + 3);
        G(b, 16*i + 4, 16*i + 5, 16*i + 6, 16*i + 7);
        G(b, 16*i + 8, 16*i + 9, 16*i + 10, 16*i + 11);
        G(b, 16*i + 12,16*i + 13,16*i + 14,16*i + 15);
    }
    // Blake2b round across rows (columns)
    for (int i = 0; i < 8; ++i) {
        G(b, 2*i,      2*i + 1, 2*i + 16, 2*i + 17);
        G(b, 2*i + 32, 2*i + 33, 2*i + 48, 2*i + 49);
        G(b, 2*i + 64, 2*i + 65, 2*i + 80, 2*i + 81);
        G(b, 2*i + 96, 2*i + 97, 2*i + 112, 2*i + 113);
    }
}

inline void CompressBlock(const Block& X, const Block& Y, Block& out) {
    Block R;
    for (int i = 0; i < 128; ++i) R.v[i] = X.v[i] ^ Y.v[i];
    Block Z = R;
    Round(Z);
    for (int i = 0; i < 128; ++i) out.v[i] = Z.v[i] ^ R.v[i];
}

void FillBlock(const Block& prev, const Block& ref, Block& out, bool with_xor) {
    Block tmp;
    CompressBlock(prev, ref, tmp);
    if (with_xor) for (int i = 0; i < 128; ++i) out.v[i] ^= tmp.v[i];
    else          for (int i = 0; i < 128; ++i) out.v[i] = tmp.v[i];
}

inline uint32_t IndexAlpha(uint32_t pass, uint32_t slice, uint32_t lane, uint32_t index,
                           uint32_t lane_len, uint32_t seg_len, uint32_t pseudo_rand,
                           bool same_lane) {
    uint32_t ref_area_size;
    if (pass == 0) {
        if (slice == 0) ref_area_size = index - 1;
        else if (same_lane) ref_area_size = slice * seg_len + index - 1;
        else ref_area_size = slice * seg_len + (index == 0 ? -1 : 0);
    } else {
        if (same_lane) ref_area_size = lane_len - seg_len + index - 1;
        else ref_area_size = lane_len - seg_len + (index == 0 ? -1 : 0);
    }

    uint64_t rel = pseudo_rand;
    rel = (rel * rel) >> 32;
    rel = (uint64_t)ref_area_size - 1 - (((uint64_t)ref_area_size * rel) >> 32);

    uint32_t start = 0;
    if (pass != 0) {
        start = (slice == ARGON2_SYNC_POINTS - 1) ? 0 : (slice + 1) * seg_len;
    }
    return (uint32_t)((start + rel) % lane_len);
}

std::vector<uint8_t> HPrime(uint32_t outlen, const std::vector<uint8_t>& input) {
    // H'^{outlen}
    std::vector<uint8_t> out;
    out.reserve(outlen);
    std::vector<uint8_t> le32(4);
    le32[0] = (uint8_t)outlen;
    le32[1] = (uint8_t)(outlen >> 8);
    le32[2] = (uint8_t)(outlen >> 16);
    le32[3] = (uint8_t)(outlen >> 24);

    std::vector<uint8_t> v;
    v.insert(v.end(), le32.begin(), le32.end());
    v.insert(v.end(), input.begin(), input.end());

    uint8_t buf[64];
    Blake2b(v.data(), v.size(), buf, 64);
    out.insert(out.end(), buf, buf + 32);
    if (outlen <= 64) {
        out.resize(outlen);
        return out;
    }
    v.assign(buf, buf + 64);
    while (out.size() < outlen - 64) {
        Blake2b(v.data(), v.size(), buf, 64);
        out.insert(out.end(), buf, buf + 32);
        v.assign(buf, buf + 64);
    }
    Blake2b(v.data(), v.size(), buf, 64);
    size_t need = outlen - out.size();
    out.insert(out.end(), buf, buf + need);
    return out;
}

} // namespace

std::vector<uint8_t> Argon2id(const std::vector<uint8_t>& password,
                              const std::vector<uint8_t>& salt,
                              const Argon2Params& params) {
    const uint32_t p = params.parallelism;
    const uint32_t t = params.t_cost;
    uint32_t m = params.m_cost;

    // Minimum memory: 8 * p blocks.
    if (m < 8 * p) m = 8 * p;
    // Segment length.
    uint32_t seg_len = m / (p * ARGON2_SYNC_POINTS);
    uint32_t lane_len = seg_len * ARGON2_SYNC_POINTS;
    // Adjust total.
    uint32_t total = lane_len * p;

    // ---- H0 ----
    auto LE32 = [](uint32_t v) {
        std::vector<uint8_t> out(4);
        out[0] = (uint8_t)v; out[1] = (uint8_t)(v >> 8);
        out[2] = (uint8_t)(v >> 16); out[3] = (uint8_t)(v >> 24);
        return out;
    };
    std::vector<uint8_t> H0_input;
    auto append = [&](const std::vector<uint8_t>& v) {
        H0_input.insert(H0_input.end(), v.begin(), v.end());
    };
    append(LE32(p)); append(LE32(params.hash_len)); append(LE32(m)); append(LE32(t));
    append(LE32(ARGON2_VERSION)); append(LE32(2)); // type=2 for Argon2id
    append(LE32((uint32_t)password.size())); append(password);
    append(LE32((uint32_t)salt.size())); append(salt);
    append(LE32(0)); // no secret
    append(LE32(0)); // no associated data

    uint8_t h0[64];
    Blake2b(H0_input.data(), H0_input.size(), h0, 64);

    // ---- Allocate memory ----
    std::vector<Block> B(total);
    // First 2 columns.
    for (uint32_t i = 0; i < p; ++i) {
        std::vector<uint8_t> seed(72);
        memcpy(seed.data(), h0, 64);
        // LE32(i) + LE32(0/1)
        auto put32 = [&](size_t off, uint32_t v) {
            seed[off] = (uint8_t)v; seed[off+1] = (uint8_t)(v >> 8);
            seed[off+2] = (uint8_t)(v >> 16); seed[off+3] = (uint8_t)(v >> 24);
        };
        put32(64, 0); put32(68, i);
        auto b0 = HPrime(1024, seed);
        memcpy(B[i * lane_len + 0].v, b0.data(), 1024);
        put32(64, 1);
        auto b1 = HPrime(1024, seed);
        memcpy(B[i * lane_len + 1].v, b1.data(), 1024);
    }

    // ---- Fill ----
    for (uint32_t pass = 0; pass < t; ++pass) {
        for (uint32_t slice = 0; slice < ARGON2_SYNC_POINTS; ++slice) {
            for (uint32_t lane = 0; lane < p; ++lane) {
                for (uint32_t idx = 0; idx < seg_len; ++idx) {
                    uint32_t cur = lane * lane_len + slice * seg_len + idx;
                    if (pass == 0 && slice == 0 && idx < 2) continue; // already set
                    uint32_t prev = (cur % lane_len == 0) ? cur + lane_len - 1 : cur - 1;

                    Block J;
                    J.v[0] = pass; J.v[1] = lane; J.v[2] = slice; J.v[3] = lane_len;
                    J.v[4] = total; J.v[5] = t; J.v[6] = 2;
                    uint8_t jbuf[1024];
                    for (int k = 0; k < 8; ++k) memcpy(jbuf + k * 8, &J.v[k], 8);
                    Block out;
                    FillBlock(B[prev], B[cur], out, false);
                    // re-use: we compute J1||J2 by hashing B[prev]
                    uint8_t prevbuf[1024];
                    for (int k = 0; k < 128; ++k) memcpy(prevbuf + k * 8, &B[prev].v[k], 8);
                    uint8_t hprev[64];
                    Blake2b(prevbuf, 1024, hprev, 64);
                    uint32_t J1 = (uint32_t)hprev[0] | ((uint32_t)hprev[1] << 8) |
                                  ((uint32_t)hprev[2] << 16) | ((uint32_t)hprev[3] << 24);
                    uint32_t J2 = (uint32_t)hprev[4] | ((uint32_t)hprev[5] << 8) |
                                  ((uint32_t)hprev[6] << 16) | ((uint32_t)hprev[7] << 24);
                    uint32_t ref_lane = J2 % p;
                    bool same_lane = (ref_lane == lane);
                    uint32_t ref_idx = IndexAlpha(pass, slice, lane, idx, lane_len, seg_len, J1, same_lane);
                    uint32_t ref_pos = ref_lane * lane_len + ref_idx;
                    bool with_xor = (pass > 0);
                    FillBlock(B[prev], B[ref_pos], B[cur], with_xor);
                }
            }
        }
    }

    // ---- Finalize ----
    Block final_block = B[lane_len - 1];
    for (uint32_t lane = 1; lane < p; ++lane) {
        const Block& last = B[lane * lane_len + lane_len - 1];
        for (int i = 0; i < 128; ++i) final_block.v[i] ^= last.v[i];
    }
    uint8_t finalbuf[1024];
    for (int k = 0; k < 128; ++k) memcpy(finalbuf + k * 8, &final_block.v[k], 8);
    return HPrime(params.hash_len, std::vector<uint8_t>(finalbuf, finalbuf + 1024));
}

} // namespace shivrai
