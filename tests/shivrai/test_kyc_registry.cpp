#include "shivrai/kyc_registry/kyc_registry.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::kyc_registry;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    KycRegistry k;
    CHECK(k.Count() == 0);
    CHECK(!k.Register("", KycLevel::Basic, "IN", "v", 100));
    CHECK(!k.Register("alice", KycLevel::Basic, "", "v", 100));
    CHECK(!k.Register("alice", KycLevel::Basic, "IN", "", 100));
    CHECK(!k.Register("alice", KycLevel::Basic, "IN", "v", 0));
    CHECK(k.Register("alice", KycLevel::Basic, "IN", "verifier1", 3600));
    CHECK(k.Register("bob", KycLevel::Verified, "US", "verifier1", 3600));
    CHECK(k.Count() == 2);
    CHECK(!k.Register("alice", KycLevel::Verified, "IN", "v", 3600));
    auto a = k.Get("alice");
    CHECK(a.has_value());
    CHECK(a->level == KycLevel::Basic);
    CHECK(a->jurisdiction == "IN");
    CHECK(a->verifier == "verifier1");
    CHECK(a->revoked == false);
    CHECK(k.IsValid("alice", KycLevel::Basic));
    CHECK(!k.IsValid("alice", KycLevel::Verified));
    CHECK(k.IsValid("bob", KycLevel::Verified));
    CHECK(k.IsValid("bob", KycLevel::Basic));
    CHECK(k.IsValid("bob", KycLevel::Institutional) == false);
    CHECK(!k.IsValid("carol", KycLevel::Basic));
    CHECK(k.LevelOf("alice") == KycLevel::Basic);
    CHECK(k.LevelOf("bob") == KycLevel::Verified);
    CHECK(k.LevelOf("carol") == KycLevel::None);
    CHECK(k.Upgrade("alice", KycLevel::Verified, "verifier2"));
    CHECK(k.LevelOf("alice") == KycLevel::Verified);
    CHECK(!k.Upgrade("alice", KycLevel::Basic, "v"));  // cannot downgrade
    CHECK(!k.Upgrade("carol", KycLevel::Basic, "v"));
    CHECK(!k.Upgrade("alice", KycLevel::Institutional, ""));
    CHECK(k.SetJurisdiction("alice", "SG"));
    CHECK(k.Get("alice")->jurisdiction == "SG");
    CHECK(!k.SetJurisdiction("alice", ""));
    CHECK(!k.SetJurisdiction("carol", "IN"));
    CHECK(k.Revoke("alice", "verifier1"));
    CHECK(k.Get("alice")->revoked);
    CHECK(!k.IsValid("alice", KycLevel::Basic));
    CHECK(k.LevelOf("alice") == KycLevel::None);
    CHECK(!k.Revoke("alice", "v"));
    CHECK(!k.Upgrade("alice", KycLevel::Institutional, "v"));
    CHECK(k.ValidCount(KycLevel::Basic) == 1);
    CHECK(k.ValidCount(KycLevel::Verified) == 1);
    CHECK(k.ValidCount(KycLevel::Institutional) == 0);
    CHECK(!k.Get("nope").has_value());
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
