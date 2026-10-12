#include "shivrai/wallet/blake2b.h"
#include <cstring>
#include <array>

namespace shivrai {

static const uint64_t IV[8] = {
    0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
    0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
    0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
    0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
};

static const uint8_t SIGMA[12][16] = {
    {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15},
    {14,10,4,8,9,15,13,6,1,12,0,2,11,7,5,3},
    {11,8,12,0,5,2,15,13,10,14,3,6,7,1,9,4},
    {7,9,3,1,13,12,11,14,2,6,5,10,4,0,15,8},
    {9,0,5,7,2,4,10,15,14,1,11,12,6,8,3,13},
    {2,12,6,10,0,11,8,3,4,13,7,5,15,14,1,9},
    {12,5,1,15,14,13,4,10,0,7,6,3,9,2,8,11},
    {13,11,7,14,12,1,3,9,5,0,15,4,8,6,2,10},
    {6,15,14,9,11,3,0,8,12,2,13,7,1,4,10,5},
    {10,2,8,4,7,6,1,5,15,11,9,14,3,12,13,0},
    {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15},
    {14,10,4,8,9,15,13,6,1,12,0,2,11,7,5,3}
};

static inline uint64_t RotR(uint64_t x, int n) { return (x >> n) | (x << (64 - n)); }

#define G(r,i,a,b,c,d) \
    a = a + b + m[SIGMA[r][2*i]]; \
    d = RotR(d ^ a, 32); \
    c = c + d; \
    b = RotR(b ^ c, 24); \
    a = a + b + m[SIGMA[r][2*i+1]]; \
    d = RotR(d ^ a, 16); \
    c = c + d; \
    b = RotR(b ^ c, 63);

struct B2bState {
    uint64_t h[8];
    uint64_t t[2];
    uint64_t f[2];
    uint8_t buf[128];
    size_t buflen;
    size_t outlen;
};

static void Compress(B2bState* S, const uint8_t block[128]) {
    uint64_t m[16];
    for (int i = 0; i < 16; ++i) {
        memcpy(&m[i], block + i * 8, 8);
    }
    uint64_t v[16];
    for (int i = 0; i < 8; ++i) v[i] = S->h[i];
    for (int i = 0; i < 8; ++i) v[i + 8] = IV[i];
    v[12] ^= S->t[0];
    v[13] ^= S->t[1];
    v[14] ^= S->f[0];
    v[15] ^= S->f[1];

    for (int r = 0; r < 12; ++r) {
        G(r,0,v[0],v[4],v[8],v[12]);
        G(r,1,v[1],v[5],v[9],v[13]);
        G(r,2,v[2],v[6],v[10],v[14]);
        G(r,3,v[3],v[7],v[11],v[15]);
        G(r,4,v[0],v[5],v[10],v[15]);
        G(r,5,v[1],v[6],v[11],v[12]);
        G(r,6,v[2],v[7],v[8],v[13]);
        G(r,7,v[3],v[4],v[9],v[14]);
    }
    for (int i = 0; i < 8; ++i) S->h[i] ^= v[i] ^ v[i + 8];
}

static void Init(B2bState* S, size_t outlen, const uint8_t* key, size_t keylen) {
    for (int i = 0; i < 8; ++i) S->h[i] = IV[i];
    S->h[0] ^= 0x01010000ULL ^ ((uint64_t)keylen << 8) ^ (uint64_t)outlen;
    S->t[0] = S->t[1] = 0;
    S->f[0] = S->f[1] = 0;
    S->buflen = 0;
    S->outlen = outlen;
    if (keylen > 0) {
        uint8_t block[128] = {0};
        memcpy(block, key, keylen);
        // absorb as first block
        memcpy(S->buf, block, 128);
        S->buflen = 128;
        // Process it as a full block: this is standard for keyed blake2b.
        // For simplicity we absorb key as a block by not compressing now,
        // but marking buffer full so next Update flushes.
    }
}

static void Update(B2bState* S, const uint8_t* in, size_t inlen) {
    while (inlen > 0) {
        if (S->buflen == 128) {
            S->t[0] += 128;
            if (S->t[0] < 128) S->t[1]++;
            Compress(S, S->buf);
            S->buflen = 0;
        }
        size_t take = 128 - S->buflen;
        if (take > inlen) take = inlen;
        memcpy(S->buf + S->buflen, in, take);
        S->buflen += take;
        in += take;
        inlen -= take;
    }
}

static void Final(B2bState* S, uint8_t* out) {
    S->t[0] += S->buflen;
    if (S->t[0] < S->buflen) S->t[1]++;
    S->f[0] = ~0ULL;
    memset(S->buf + S->buflen, 0, 128 - S->buflen);
    Compress(S, S->buf);
    uint8_t full[64];
    for (int i = 0; i < 8; ++i) memcpy(full + i * 8, &S->h[i], 8);
    memcpy(out, full, S->outlen);
}

void Blake2b(const uint8_t* in, size_t inlen, uint8_t* out, size_t outlen) {
    Blake2bKeyed(in, inlen, nullptr, 0, out, outlen);
}

void Blake2bKeyed(const uint8_t* in, size_t inlen, const uint8_t* key, size_t keylen,
                  uint8_t* out, size_t outlen) {
    B2bState S;
    Init(&S, outlen, key, keylen);
    Update(&S, in, inlen);
    Final(&S, out);
}

} // namespace shivrai
