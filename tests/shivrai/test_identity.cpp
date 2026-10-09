#include "test_framework.h"
#include "shivrai/identity/identity.h"
using namespace shivrai::identity;

TEST(register_did_valid) {
    IdentityRegistry reg;
    ASSERT_TRUE(reg.register_did("did:alice", "alice", "0xABC"));
    ASSERT_TRUE(reg.is_active("did:alice"));
}

TEST(register_did_duplicate_fails) {
    IdentityRegistry reg;
    reg.register_did("did:alice", "alice", "0xABC");
    ASSERT_FALSE(reg.register_did("did:alice", "alice", "0xDEF"));
}

TEST(register_did_empty_fails) {
    IdentityRegistry reg;
    ASSERT_FALSE(reg.register_did("", "a", "0x1"));
    ASSERT_FALSE(reg.register_did("d", "", "0x1"));
    ASSERT_FALSE(reg.register_did("d", "a", ""));
}

TEST(deactivate_did) {
    IdentityRegistry reg;
    reg.register_did("did:alice", "alice", "0xABC");
    ASSERT_TRUE(reg.deactivate_did("did:alice"));
    ASSERT_FALSE(reg.is_active("did:alice"));
}

TEST(issue_and_verify_credential) {
    IdentityRegistry reg;
    reg.register_did("did:issuer", "i", "0x1");
    auto c = reg.issue_credential("did:issuer", "did:bob", "KYC", "v", 3600);
    ASSERT_FALSE(c.empty());
    ASSERT_TRUE(reg.verify_credential(c));
}

TEST(revoke_credential) {
    IdentityRegistry reg;
    reg.register_did("did:issuer", "i", "0x1");
    auto c = reg.issue_credential("did:issuer", "did:bob", "KYC", "v", 3600);
    ASSERT_TRUE(reg.revoke_credential(c));
    ASSERT_FALSE(reg.verify_credential(c));
}

TEST(kyc_lifecycle) {
    IdentityRegistry reg;
    reg.register_did("did:alice", "a", "0x1");
    ASSERT_FALSE(reg.is_kyc_verified("did:alice"));
    ASSERT_TRUE(reg.set_kyc("did:alice", "level2", "v1"));
    ASSERT_TRUE(reg.is_kyc_verified("did:alice"));
}

TEST(credentials_by_subject) {
    IdentityRegistry reg;
    reg.register_did("did:issuer", "i", "0x1");
    reg.issue_credential("did:issuer", "did:bob", "KYC", "v1", 0);
    reg.issue_credential("did:issuer", "did:bob", "AML", "v2", 0);
    ASSERT_EQ(reg.credentials_by_subject("did:bob").size(), 2u);
}

int main() {
    return shivrai::test::TestRunner::instance().run();
}
