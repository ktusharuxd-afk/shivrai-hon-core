#include "test_framework.h"
#include "shivrai/evm_compat.h"

using namespace shivrai::evm;

TEST(evm_chain_id) {
    ASSERT_EQ(GlobalEVMNetworkConfig().chain_id, 2026001u);
}

TEST(evm_default_gas_limit) {
    ASSERT_EQ(GlobalEVMNetworkConfig().default_gas_limit, 30'000'000u);
}

TEST(evm_max_gas) {
    ASSERT_EQ(GlobalEVMNetworkConfig().max_gas_per_block, 30'000'000u);
}

TEST(evm_target_gas) {
    ASSERT_EQ(GlobalEVMNetworkConfig().target_gas_per_block, 15'000'000u);
}

TEST(evm_gas_floor) {
    ASSERT_EQ(GlobalEVMNetworkConfig().gas_price_floor_hon_nano, 1u);
}

TEST(evm_execution_not_live) {
    ASSERT_FALSE(GlobalEVMNetworkConfig().execution_live);
}

TEST(evm_status_foundation_only) {
    ASSERT_EQ(std::string(EVMExecutionStatus()), std::string("foundation-only"));
}

TEST(address_zero_all_bytes_zero) {
    Address z = ZeroAddress();
    for (auto b : z) {
        ASSERT_EQ(b, 0);
    }
}

TEST(address_max_all_bytes_ff) {
    Address m = MaxAddress();
    for (auto b : m) {
        ASSERT_EQ(b, 0xFF);
    }
}

TEST(address_is_zero_positive) {
    ASSERT_TRUE(IsZeroAddress(ZeroAddress()));
}

TEST(address_is_zero_negative) {
    ASSERT_FALSE(IsZeroAddress(MaxAddress()));
}

TEST(address_to_hex_has_0x_prefix) {
    std::string h = AddressToHex(ZeroAddress());
    ASSERT_EQ(h.size(), 42u);
    ASSERT_EQ(h.substr(0, 2), std::string("0x"));
}

TEST(address_to_hex_zero_address) {
    std::string h = AddressToHex(ZeroAddress());
    ASSERT_EQ(h, std::string("0x0000000000000000000000000000000000000000"));
}

TEST(address_to_hex_max_address) {
    std::string h = AddressToHex(MaxAddress());
    ASSERT_EQ(h, std::string("0xffffffffffffffffffffffffffffffffffffffff"));
}

TEST(hex_to_address_roundtrip_zero) {
    Address parsed;
    std::string err;
    ASSERT_TRUE(HexToAddress("0x0000000000000000000000000000000000000000", parsed, err));
    ASSERT_TRUE(parsed == ZeroAddress());
}

TEST(hex_to_address_roundtrip_max) {
    Address parsed;
    std::string err;
    ASSERT_TRUE(HexToAddress("0xffffffffffffffffffffffffffffffffffffffff", parsed, err));
    ASSERT_TRUE(parsed == MaxAddress());
}

TEST(hex_to_address_no_prefix) {
    Address parsed;
    std::string err;
    ASSERT_TRUE(HexToAddress("0000000000000000000000000000000000000000", parsed, err));
    ASSERT_TRUE(parsed == ZeroAddress());
}

TEST(hex_to_address_invalid_length) {
    Address parsed;
    std::string err;
    ASSERT_FALSE(HexToAddress("0x123", parsed, err));
    ASSERT_FALSE(err.empty());
}

TEST(hex_to_address_invalid_chars) {
    Address parsed;
    std::string err;
    ASSERT_FALSE(HexToAddress("0xGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG", parsed, err));
    ASSERT_FALSE(err.empty());
}

TEST(validate_envelope_valid) {
    EVMTransactionEnvelope tx{};
    tx.chain_id = 2026001;
    tx.gas_limit = 21000;
    tx.max_fee_per_gas_nano = 100;
    tx.max_priority_fee_per_gas_nano = 5;
    std::string err;
    ASSERT_TRUE(ValidateEnvelope(tx, err));
}

TEST(validate_envelope_wrong_chain) {
    EVMTransactionEnvelope tx{};
    tx.chain_id = 999;
    tx.gas_limit = 21000;
    tx.max_fee_per_gas_nano = 100;
    std::string err;
    ASSERT_FALSE(ValidateEnvelope(tx, err));
}

TEST(validate_envelope_zero_gas) {
    EVMTransactionEnvelope tx{};
    tx.chain_id = 2026001;
    tx.gas_limit = 0;
    tx.max_fee_per_gas_nano = 100;
    std::string err;
    ASSERT_FALSE(ValidateEnvelope(tx, err));
}

TEST(validate_envelope_priority_exceeds_max) {
    EVMTransactionEnvelope tx{};
    tx.chain_id = 2026001;
    tx.gas_limit = 21000;
    tx.max_fee_per_gas_nano = 10;
    tx.max_priority_fee_per_gas_nano = 100;
    std::string err;
    ASSERT_FALSE(ValidateEnvelope(tx, err));
}

TEST(validate_envelope_fee_below_floor) {
    EVMTransactionEnvelope tx{};
    tx.chain_id = 2026001;
    tx.gas_limit = 21000;
    tx.max_fee_per_gas_nano = 0;
    std::string err;
    ASSERT_FALSE(ValidateEnvelope(tx, err));
}

TEST(effective_gas_price_capped_by_max) {
    EVMTransactionEnvelope tx{};
    tx.max_fee_per_gas_nano = 100;
    tx.max_priority_fee_per_gas_nano = 5;
    ASSERT_EQ(EffectiveGasPrice(tx, 50), 55u);
}

TEST(effective_gas_price_limited_by_max) {
    EVMTransactionEnvelope tx{};
    tx.max_fee_per_gas_nano = 30;
    tx.max_priority_fee_per_gas_nano = 5;
    ASSERT_EQ(EffectiveGasPrice(tx, 50), 30u);
}

TEST(max_total_cost_calculation) {
    EVMTransactionEnvelope tx{};
    tx.gas_limit = 21000;
    tx.max_fee_per_gas_nano = 100;
    tx.value_hon_nano = 1000;
    ASSERT_EQ(MaxTotalCost(tx), 2101000u);
}

TEST(max_total_cost_zero_gas) {
    EVMTransactionEnvelope tx{};
    tx.gas_limit = 0;
    ASSERT_EQ(MaxTotalCost(tx), 0u);
}

TEST(miner_tip_basic) {
    EVMTransactionEnvelope tx{};
    tx.max_fee_per_gas_nano = 100;
    tx.max_priority_fee_per_gas_nano = 5;
    ASSERT_EQ(MinerTip(tx, 50), 5u);
}

TEST(miner_tip_below_base) {
    EVMTransactionEnvelope tx{};
    tx.max_fee_per_gas_nano = 100;
    tx.max_priority_fee_per_gas_nano = 5;
    ASSERT_EQ(MinerTip(tx, 200), 0u);
}

TEST(describe_config_not_empty) {
    std::string d = DescribeEVMConfig();
    ASSERT_FALSE(d.empty());
    ASSERT_TRUE(d.find("2026001") != std::string::npos);
    ASSERT_TRUE(d.find("foundation-only") != std::string::npos);
}

int main() {
    return shivrai::test::TestRunner::instance().run();
}
