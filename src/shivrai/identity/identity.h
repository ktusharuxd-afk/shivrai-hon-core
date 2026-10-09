#ifndef SHIVRAI_HON_IDENTITY_H
#define SHIVRAI_HON_IDENTITY_H
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <mutex>

namespace shivrai::identity {

enum class CredentialStatus { ACTIVE, REVOKED, EXPIRED };

struct DID {
    std::string id;
    std::string controller;
    std::string public_key;
    uint64_t created_at{0};
    bool active{true};
};

struct Credential {
    std::string id;
    std::string issuer_did;
    std::string subject_did;
    std::string type;
    std::string value;
    uint64_t issued_at{0};
    uint64_t expires_at{0};
    CredentialStatus status{CredentialStatus::ACTIVE};
};

struct KYCRecord {
    std::string did;
    std::string level;
    bool verified{false};
    uint64_t verified_at{0};
    std::string verifier;
};

class IdentityRegistry {
public:
    bool register_did(const std::string& id, const std::string& controller,
                      const std::string& public_key);
    bool deactivate_did(const std::string& id);
    const DID* resolve(const std::string& did) const;
    bool is_active(const std::string& did) const;

    std::string issue_credential(const std::string& issuer_did,
                                  const std::string& subject_did,
                                  const std::string& type,
                                  const std::string& value,
                                  uint64_t ttl_seconds = 0);
    bool revoke_credential(const std::string& credential_id);
    const Credential* get_credential(const std::string& id) const;
    bool verify_credential(const std::string& id) const;

    bool set_kyc(const std::string& did, const std::string& level,
                 const std::string& verifier);
    const KYCRecord* get_kyc(const std::string& did) const;
    bool is_kyc_verified(const std::string& did) const;

    std::vector<Credential> credentials_by_subject(const std::string& did) const;
    size_t did_count() const { return dids_.size(); }

private:
    mutable std::recursive_mutex mutex_;
    std::unordered_map<std::string, DID> dids_;
    std::unordered_map<std::string, Credential> credentials_;
    std::unordered_map<std::string, KYCRecord> kyc_records_;
    std::string gen_id(const std::string& prefix) const;
};

} // namespace shivrai::identity
#endif
