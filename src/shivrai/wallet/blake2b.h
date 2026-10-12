#ifndef SHIVRAI_WALLET_BLAKE2B_H
#define SHIVRAI_WALLET_BLAKE2B_H

#include <cstdint>
#include <cstddef>
#include <vector>

namespace shivrai {

void Blake2b(const uint8_t* in, size_t inlen, uint8_t* out, size_t outlen);
void Blake2bKeyed(const uint8_t* in, size_t inlen, const uint8_t* key, size_t keylen,
                  uint8_t* out, size_t outlen);

} // namespace shivrai

#endif
