#ifndef SHIVRAI_WALLET_ARGON2_H
#define SHIVRAI_WALLET_ARGON2_H

#include <cstdint>
#include <cstddef>
#include <vector>

namespace shivrai {

// Argon2id (RFC 9106) parameters.
struct Argon2Params {
    uint32_t t_cost{3};        // iterations
    uint32_t m_cost{65536};    // memory in KiB (64 MiB)
    uint32_t parallelism{4};
    uint32_t hash_len{32};
};

/** Argon2id(password, salt, params) -> hash. */
std::vector<uint8_t> Argon2id(const std::vector<uint8_t>& password,
                              const std::vector<uint8_t>& salt,
                              const Argon2Params& params = Argon2Params{});

} // namespace shivrai

#endif
