#ifndef SHIVRAI_HON_UPGRADEABLE_PROXY_H
#define SHIVRAI_HON_UPGRADEABLE_PROXY_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
namespace shivrai::upgradeable_proxy {
struct Version {
    uint32_t major{0}, minor{0}, patch{0};
    std::string impl_hash;
    int64_t deployed_at{0};
    bool active{true};
};
class UpgradeableProxy {
public:
    UpgradeableProxy();
    bool Initialize(const std::string& admin, const std::string& initial_hash);
    bool Upgrade(const std::string& caller, const std::string& new_hash,
                 uint32_t major, uint32_t minor, uint32_t patch);
    bool Rollback(const std::string& caller);
    bool TransferAdmin(const std::string& caller, const std::string& new_admin);
    std::string CurrentHash() const;
    std::string Admin() const;
    size_t VersionCount() const;
    std::optional<Version> VersionAt(size_t idx) const;
    bool IsInitialized() const;
private:
    mutable std::recursive_mutex cs_;
    std::string admin_;
    std::string current_hash_;
    std::vector<Version> history_;
    bool initialized_{false};
};
}
#endif
