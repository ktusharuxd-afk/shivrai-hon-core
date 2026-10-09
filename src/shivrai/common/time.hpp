#ifndef SHIVRAI_COMMON_TIME_HPP
#define SHIVRAI_COMMON_TIME_HPP

#include <chrono>
#include <cstdint>

namespace shivrai::common {

// Unix timestamp in SECONDS — persistent timestamps साठी
inline uint64_t now_seconds() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

// Unix timestamp in MILLISECONDS — जिथे precision हवी
inline uint64_t now_millis() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

// Monotonic nanoseconds — RNG seeds साठी
inline uint64_t monotonic_ns() {
    return static_cast<uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count()
    );
}

} // namespace shivrai::common

#endif
