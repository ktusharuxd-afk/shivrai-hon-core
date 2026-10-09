#include "identity.h"
#include "shivrai/common/time.hpp"
#include <random>
#include <sstream>

namespace shivrai::identity {

std::string IdentityRegistry::gen_id(const std::string& prefix) const {
    static std::mt19937_64 rng(shivrai::common::monotonic_ns());
    std::ostringstream ss;
    ss << prefix << "_" << rng();
    return ss.str();
}

bool IdentityRegistry::register_did(const std::string& id,
                                     const std::string& controller,
                                     const std::string& public_key) {
    if (id.empty() || controller.empty() || public_key.empty()) return false;
    if (dids_.count(id)) return false;
    DID d;
    d.id = id;
    d.controller = controller;
    d.public_key = public_key;
    d.created_at = shivrai::common::now_seconds();
    d.active = true;
    dids_[id] = d;
    return true;
}

bool IdentityRegistry::deactivate_did(const std::string& id) {
    auto it = dids_.find(id);
    if (it == dids_.end()) return false;
    it->second.active = false;
    return true;
}

const DID* IdentityRegistry::resolve(const std::string& did) const {
    auto it = dids_.find(did);
    return it == dids_.end() ? nullptr : &it->second;
}

bool IdentityRegistry::is_active(const std::string& did) const {
    auto d = resolve(did);
    return d && d->active;
}

std::string IdentityRegistry::issue_credential(const std::string& issuer_did,
                                                 const std::string& subject_did,
                                                 const std::string& type,
                                                 const std::string& value,
                                                 uint64_t ttl_seconds) {
    if (!is_active(issuer_did) || subject_did.empty()) return "";
    Credential c;
    c.id = gen_id("CRED");
    c.issuer_did = issuer_did;
    c.subject_did = subject_did;
    c.type = type;
    c.value = value;
    c.issued_at = shivrai::common::now_seconds();
    c.expires_at = ttl_seconds > 0 ? c.issued_at + ttl_seconds : 0;
    c.status = CredentialStatus::ACTIVE;
    credentials_[c.id] = c;
    return c.id;
}

bool IdentityRegistry::revoke_credential(const std::string& id) {
    auto it = credentials_.find(id);
    if (it == credentials_.end()) return false;
    it->second.status = CredentialStatus::REVOKED;
    return true;
}

const Credential* IdentityRegistry::get_credential(const std::string& id) const {
    auto it = credentials_.find(id);
    return it == credentials_.end() ? nullptr : &it->second;
}

bool IdentityRegistry::verify_credential(const std::string& id) const {
    auto c = get_credential(id);
    if (!c || c->status != CredentialStatus::ACTIVE) return false;
    if (c->expires_at > 0) {
        uint64_t now = shivrai::common::now_seconds();
        if (now > c->expires_at) return false;
    }
    return is_active(c->issuer_did);
}

bool IdentityRegistry::set_kyc(const std::string& did, const std::string& level,
                                 const std::string& verifier) {
    if (!is_active(did)) return false;
    KYCRecord r;
    r.did = did;
    r.level = level;
    r.verified = true;
    r.verified_at = shivrai::common::now_seconds();
    r.verifier = verifier;
    kyc_records_[did] = r;
    return true;
}

const KYCRecord* IdentityRegistry::get_kyc(const std::string& did) const {
    auto it = kyc_records_.find(did);
    return it == kyc_records_.end() ? nullptr : &it->second;
}

bool IdentityRegistry::is_kyc_verified(const std::string& did) const {
    auto r = get_kyc(did);
    return r && r->verified;
}

std::vector<Credential> IdentityRegistry::credentials_by_subject(const std::string& did) const {
    std::vector<Credential> result;
    for (const auto& [id, c] : credentials_)
        if (c.subject_did == did) result.push_back(c);
    return result;
}

} // namespace shivrai::identity
