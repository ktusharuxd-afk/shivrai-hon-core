#ifndef SHIVRAI_HON_KYC_REGISTRY_H
#define SHIVRAI_HON_KYC_REGISTRY_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
namespace shivrai::kyc_registry {
enum class KycLevel { None = 0, Basic = 1, Verified = 2, Institutional = 3 };
struct KycRecord {
    std::string user;
    KycLevel level{KycLevel::None};
    std::string jurisdiction;
    int64_t verified_at{0};
    int64_t expires_at{0};
    bool revoked{false};
    std::string verifier;
};
class KycRegistry {
public:
    KycRegistry();
    bool Register(const std::string& user, KycLevel level,
                  const std::string& jurisdiction, const std::string& verifier,
                  int64_t validity_sec);
    bool Upgrade(const std::string& user, KycLevel new_level, const std::string& verifier);
    bool Revoke(const std::string& user, const std::string& verifier);
    std::optional<KycRecord> Get(const std::string& user) const;
    bool IsValid(const std::string& user, KycLevel min_level) const;
    KycLevel LevelOf(const std::string& user) const;
    bool SetJurisdiction(const std::string& user, const std::string& jurisdiction);
    size_t Count() const;
    size_t ValidCount(KycLevel min_level) const;
private:
    mutable std::recursive_mutex cs_;
    std::map<std::string, KycRecord> records_;
};
}
#endif
