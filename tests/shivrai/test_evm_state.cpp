#include "test_framework.h"
#include "shivrai/evm_state.h"

using namespace shivrai::evm;

TEST(statedb_initially_empty) {
    StateDB db;
    ASSERT_EQ(db.AccountCount(), 0u);
}

TEST(statedb_get_nonexistent_returns_null) {
    StateDB db;
    Address a{};
    ASSERT_NULL(db.Get(a));
}

TEST(statedb_exists_false_initially) {
    StateDB db;
    Address a{};
    ASSERT_FALSE(db.Exists(a));
}

TEST(statedb_get_or_create) {
    StateDB db;
    Address a{};
    a[0] = 1;
    Account& acc = db.GetOrCreate(a);
    acc.balance_hon_nano = 100;
    ASSERT_TRUE(db.Exists(a));
    ASSERT_EQ(db.AccountCount(), 1u);
    ASSERT_EQ(db.GetBalance(a), 100u);
}

TEST(statedb_balance_zero_for_unknown) {
    StateDB db;
    Address a{};
    ASSERT_EQ(db.GetBalance(a), 0u);
}

TEST(statedb_set_balance) {
    StateDB db;
    Address a{};
    a[0] = 1;
    db.SetBalance(a, 5000);
    ASSERT_EQ(db.GetBalance(a), 5000u);
    ASSERT_TRUE(db.Exists(a));
}

TEST(statedb_update_balance) {
    StateDB db;
    Address a{};
    a[0] = 1;
    db.SetBalance(a, 100);
    db.SetBalance(a, 200);
    ASSERT_EQ(db.GetBalance(a), 200u);
}

TEST(statedb_nonce_default_zero) {
    StateDB db;
    Address a{};
    ASSERT_EQ(db.GetNonce(a), 0u);
}

TEST(statedb_set_nonce) {
    StateDB db;
    Address a{};
    a[0] = 1;
    db.SetNonce(a, 42);
    ASSERT_EQ(db.GetNonce(a), 42u);
}

TEST(statedb_increment_nonce) {
    StateDB db;
    Address a{};
    a[0] = 1;
    db.IncrementNonce(a);
    db.IncrementNonce(a);
    db.IncrementNonce(a);
    ASSERT_EQ(db.GetNonce(a), 3u);
}

TEST(statedb_code_empty_initially) {
    StateDB db;
    Address a{};
    ASSERT_TRUE(db.GetCode(a).empty());
}

TEST(statedb_set_get_code) {
    StateDB db;
    Address a{};
    a[0] = 1;
    std::vector<uint8_t> code = {0x60, 0x01, 0x60, 0x02, 0x01};
    db.SetCode(a, code);
    ASSERT_EQ(db.GetCode(a).size(), 5u);
    ASSERT_EQ(db.GetCode(a)[0], 0x60);
}

TEST(statedb_storage_default_zero) {
    StateDB db;
    Address a{};
    a[0] = 1;
    Word key{};
    Word value = db.GetStorage(a, key);
    for (auto b : value) ASSERT_EQ(b, 0);
}

TEST(statedb_set_get_storage) {
    StateDB db;
    Address a{};
    a[0] = 1;
    Word key{};
    key[31] = 42;
    Word val{};
    val[31] = 99;
    db.SetStorage(a, key, val);
    Word retrieved = db.GetStorage(a, key);
    ASSERT_EQ(retrieved[31], 99);
}

TEST(statedb_transfer_success) {
    StateDB db;
    Address a{}, b{};
    a[0] = 1; b[0] = 2;
    db.SetBalance(a, 1000);
    std::string err;
    ASSERT_TRUE(db.Transfer(a, b, 300, err));
    ASSERT_EQ(db.GetBalance(a), 700u);
    ASSERT_EQ(db.GetBalance(b), 300u);
    ASSERT_TRUE(err.empty());
}

TEST(statedb_transfer_insufficient) {
    StateDB db;
    Address a{}, b{};
    a[0] = 1; b[0] = 2;
    db.SetBalance(a, 100);
    std::string err;
    ASSERT_FALSE(db.Transfer(a, b, 500, err));
    ASSERT_FALSE(err.empty());
    ASSERT_EQ(db.GetBalance(a), 100u);
    ASSERT_EQ(db.GetBalance(b), 0u);
}

TEST(statedb_transfer_zero) {
    StateDB db;
    Address a{}, b{};
    a[0] = 1; b[0] = 2;
    std::string err;
    ASSERT_TRUE(db.Transfer(a, b, 0, err));
    ASSERT_EQ(db.GetBalance(a), 0u);
    ASSERT_EQ(db.GetBalance(b), 0u);
}

TEST(statedb_transfer_exact_balance) {
    StateDB db;
    Address a{}, b{};
    a[0] = 1; b[0] = 2;
    db.SetBalance(a, 500);
    std::string err;
    ASSERT_TRUE(db.Transfer(a, b, 500, err));
    ASSERT_EQ(db.GetBalance(a), 0u);
    ASSERT_EQ(db.GetBalance(b), 500u);
}

TEST(statedb_clear) {
    StateDB db;
    Address a{};
    a[0] = 1;
    db.SetBalance(a, 100);
    ASSERT_EQ(db.AccountCount(), 1u);
    db.Clear();
    ASSERT_EQ(db.AccountCount(), 0u);
}

TEST(statedb_state_hash_empty_consistent) {
    StateDB db;
    uint64_t h1 = db.StateHash();
    uint64_t h2 = db.StateHash();
    ASSERT_EQ(h1, h2);
}

TEST(statedb_state_hash_changes_on_balance) {
    StateDB db;
    Address a{};
    a[0] = 1;
    uint64_t before = db.StateHash();
    db.SetBalance(a, 100);
    uint64_t after = db.StateHash();
    ASSERT_NE(before, after);
}

TEST(statedb_state_hash_changes_on_nonce) {
    StateDB db;
    Address a{};
    a[0] = 1;
    db.SetBalance(a, 100);
    uint64_t before = db.StateHash();
    db.SetNonce(a, 5);
    uint64_t after = db.StateHash();
    ASSERT_NE(before, after);
}

TEST(statedb_state_hash_deterministic) {
    StateDB db1, db2;
    Address a{};
    a[0] = 1;
    db1.SetBalance(a, 500);
    db1.SetNonce(a, 3);
    db2.SetBalance(a, 500);
    db2.SetNonce(a, 3);
    ASSERT_EQ(db1.StateHash(), db2.StateHash());
}

// ═══════════════════════════════════════════════════════
// ExecuteBytecode tests
// ═══════════════════════════════════════════════════════

TEST(exec_empty_code_success) {
    std::vector<uint8_t> code;
    auto r = ExecuteBytecode(code, 1000);
    ASSERT_TRUE(r.success);
    ASSERT_FALSE(r.reverted);
}

TEST(exec_stop) {
    std::vector<uint8_t> code = {0x00};  // STOP
    auto r = ExecuteBytecode(code, 1000);
    ASSERT_TRUE(r.success);
}

TEST(exec_push1_stop) {
    std::vector<uint8_t> code = {0x60, 0x42, 0x00};  // PUSH1 0x42, STOP
    auto r = ExecuteBytecode(code, 1000);
    ASSERT_TRUE(r.success);
}

TEST(exec_add) {
    // PUSH1 5, PUSH1 3, ADD, STOP
    std::vector<uint8_t> code = {0x60, 0x05, 0x60, 0x03, 0x01, 0x00};
    auto r = ExecuteBytecode(code, 1000);
    ASSERT_TRUE(r.success);
}

TEST(exec_mul) {
    std::vector<uint8_t> code = {0x60, 0x04, 0x60, 0x05, 0x02, 0x00};
    auto r = ExecuteBytecode(code, 1000);
    ASSERT_TRUE(r.success);
}

TEST(exec_sub) {
    std::vector<uint8_t> code = {0x60, 0x05, 0x60, 0x03, 0x03, 0x00};
    auto r = ExecuteBytecode(code, 1000);
    ASSERT_TRUE(r.success);
}

TEST(exec_out_of_gas) {
    // PUSH1 (3 gas) + PUSH1 (3 gas) = 6 gas needed, only 3 given
    std::vector<uint8_t> code = {0x60, 0x05, 0x60, 0x03, 0x01, 0x00};
    auto r = ExecuteBytecode(code, 3);
    ASSERT_FALSE(r.success);
    ASSERT_TRUE(r.error.find("gas") != std::string::npos);
}

TEST(exec_stack_underflow_add) {
    // ADD without enough stack
    std::vector<uint8_t> code = {0x01};
    auto r = ExecuteBytecode(code, 1000);
    ASSERT_FALSE(r.success);
    ASSERT_TRUE(r.error.find("underflow") != std::string::npos);
}

TEST(exec_stack_underflow_pop) {
    std::vector<uint8_t> code = {0x50};  // POP with empty stack
    auto r = ExecuteBytecode(code, 1000);
    ASSERT_FALSE(r.success);
    ASSERT_TRUE(r.error.find("underflow") != std::string::npos);
}

TEST(exec_truncated_push1) {
    std::vector<uint8_t> code = {0x60};  // PUSH1 without operand
    auto r = ExecuteBytecode(code, 1000);
    ASSERT_FALSE(r.success);
    ASSERT_TRUE(r.error.find("truncated") != std::string::npos);
}

TEST(exec_unsupported_opcode) {
    std::vector<uint8_t> code = {0xAA};  // Not implemented
    auto r = ExecuteBytecode(code, 1000);
    ASSERT_FALSE(r.success);
    ASSERT_TRUE(r.error.find("unsupported") != std::string::npos);
}

TEST(exec_revert) {
    // PUSH1 0 (size), PUSH1 0 (offset), REVERT
    std::vector<uint8_t> code = {0x60, 0x00, 0x60, 0x00, 0xfd};
    auto r = ExecuteBytecode(code, 1000);
    ASSERT_FALSE(r.success);
    ASSERT_TRUE(r.reverted);
}

TEST(exec_return) {
    // PUSH1 0 (size), PUSH1 0 (offset), RETURN
    std::vector<uint8_t> code = {0x60, 0x00, 0x60, 0x00, 0xf3};
    auto r = ExecuteBytecode(code, 1000);
    ASSERT_TRUE(r.success);
    ASSERT_FALSE(r.reverted);
}

TEST(exec_gas_used_positive) {
    std::vector<uint8_t> code = {0x60, 0x01, 0x00};
    auto r = ExecuteBytecode(code, 1000);
    ASSERT_TRUE(r.success);
    ASSERT_TRUE(r.gas_used > 0);
}

int main() {
    return shivrai::test::TestRunner::instance().run();
}
