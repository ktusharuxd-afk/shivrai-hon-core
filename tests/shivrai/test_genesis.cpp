#include "test_framework.h"
#include "shivrai/genesis.h"

using namespace shivrai;

TEST(genesis_chain_name) {
    ASSERT_EQ(GlobalGenesisSpec().chain_name, std::string("shivrai-mainnet-v2"));
}

TEST(genesis_message_not_empty) {
    ASSERT_FALSE(GlobalGenesisSpec().message.empty());
}

TEST(genesis_version) {
    ASSERT_EQ(GlobalGenesisSpec().version, 1u);
}

TEST(genesis_initial_reward) {
    ASSERT_EQ(GlobalGenesisSpec().initial_reward_hon, 2000u);
}

TEST(genesis_bits) {
    ASSERT_EQ(GlobalGenesisSpec().bits, 0x1d00ffffu);
}

TEST(genesis_not_frozen_yet) {
    ASSERT_FALSE(IsGenesisFrozen());
    ASSERT_FALSE(GlobalGenesisSpec().frozen);
}

TEST(validate_default_spec) {
    std::string err;
    ASSERT_TRUE(ValidateGenesisSpec(GlobalGenesisSpec(), err));
    ASSERT_TRUE(err.empty());
}

TEST(validate_rejects_empty_chain_name) {
    GenesisSpec spec = GlobalGenesisSpec();
    spec.chain_name = "";
    std::string err;
    ASSERT_FALSE(ValidateGenesisSpec(spec, err));
    ASSERT_FALSE(err.empty());
}

TEST(validate_rejects_empty_message) {
    GenesisSpec spec = GlobalGenesisSpec();
    spec.message = "";
    std::string err;
    ASSERT_FALSE(ValidateGenesisSpec(spec, err));
}

TEST(validate_rejects_zero_bits) {
    GenesisSpec spec = GlobalGenesisSpec();
    spec.bits = 0;
    std::string err;
    ASSERT_FALSE(ValidateGenesisSpec(spec, err));
}

TEST(validate_rejects_zero_version) {
    GenesisSpec spec = GlobalGenesisSpec();
    spec.version = 0;
    std::string err;
    ASSERT_FALSE(ValidateGenesisSpec(spec, err));
}

TEST(validate_rejects_zero_reward) {
    GenesisSpec spec = GlobalGenesisSpec();
    spec.initial_reward_hon = 0;
    std::string err;
    ASSERT_FALSE(ValidateGenesisSpec(spec, err));
}

TEST(validate_frozen_requires_timestamp) {
    GenesisSpec spec = GlobalGenesisSpec();
    spec.frozen = true;
    spec.timestamp = 0;
    std::string err;
    ASSERT_FALSE(ValidateGenesisSpec(spec, err));
}

TEST(validate_frozen_requires_nonce) {
    GenesisSpec spec = GlobalGenesisSpec();
    spec.frozen = true;
    spec.timestamp = 1735689600;
    spec.nonce = 0;
    std::string err;
    ASSERT_FALSE(ValidateGenesisSpec(spec, err));
}

TEST(validate_frozen_valid) {
    GenesisSpec spec = GlobalGenesisSpec();
    spec.frozen = true;
    spec.timestamp = 1735689600;
    spec.nonce = 12345;
    std::string err;
    ASSERT_TRUE(ValidateGenesisSpec(spec, err));
}

TEST(genesis_hash_not_empty) {
    ASSERT_FALSE(GenesisHashHex().empty());
}

TEST(genesis_hash_consistent) {
    ASSERT_EQ(GenesisHashHex(), GenesisHashHex());
}

TEST(genesis_hash_is_16_hex_chars) {
    ASSERT_EQ(GenesisHashHex().size(), 16u);
}

TEST(matches_genesis_hash_positive) {
    ASSERT_TRUE(MatchesGenesisHash(GenesisHashHex()));
}

TEST(matches_genesis_hash_negative) {
    ASSERT_FALSE(MatchesGenesisHash("0000000000000000"));
    ASSERT_FALSE(MatchesGenesisHash(""));
    ASSERT_FALSE(MatchesGenesisHash("deadbeefdeadbeef"));
}

TEST(describe_genesis_not_empty) {
    std::string desc = DescribeGenesis();
    ASSERT_FALSE(desc.empty());
}

TEST(describe_contains_chain_name) {
    std::string desc = DescribeGenesis();
    ASSERT_TRUE(desc.find("shivrai-mainnet-v2") != std::string::npos);
}

TEST(describe_contains_reward) {
    std::string desc = DescribeGenesis();
    ASSERT_TRUE(desc.find("2000") != std::string::npos);
}

TEST(describe_contains_frozen_status) {
    std::string desc = DescribeGenesis();
    ASSERT_TRUE(desc.find("Frozen") != std::string::npos);
}

int main() {
    return shivrai::test::TestRunner::instance().run();
}
