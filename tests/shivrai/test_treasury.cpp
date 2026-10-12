#include "test_framework.h"
#include "shivrai/treasury/treasury.h"

using namespace shivrai::treasury;

TEST(fee_allocation_default_valid) {
    FeeAllocation a;
    ASSERT_TRUE(a.valid());
}

TEST(fee_allocation_sum_must_be_100) {
    FeeAllocation a;
    a.security_percent = 60;  // Total = 110, invalid
    ASSERT_FALSE(a.valid());
}

TEST(fee_allocation_zero_sum_invalid) {
    FeeAllocation a;
    a.security_percent = 0;
    a.burn_percent = 0;
    a.ecosystem_percent = 0;
    a.developer_percent = 0;
    a.reserve_percent = 0;
    ASSERT_FALSE(a.valid());
}

TEST(allocate_fee_10000) {
    FeeAllocation a;
    auto r = AllocateFee(10000, a);
    ASSERT_EQ(r.security, 5000u);
    ASSERT_EQ(r.burn, 2000u);
    ASSERT_EQ(r.ecosystem, 1500u);
    ASSERT_EQ(r.developer, 1000u);
    ASSERT_EQ(r.reserve, 500u);
    ASSERT_EQ(r.accounted, 10000u);
}

TEST(allocate_fee_zero) {
    FeeAllocation a;
    auto r = AllocateFee(0, a);
    ASSERT_EQ(r.accounted, 0u);
}

TEST(allocate_fee_invalid_allocation) {
    FeeAllocation a;
    a.security_percent = 200;  // Invalid
    auto r = AllocateFee(1000, a);
    ASSERT_EQ(r.accounted, 0u);
}

TEST(allocate_fee_small_amount) {
    FeeAllocation a;
    auto r = AllocateFee(1, a);
    // All rounding goes to security
    ASSERT_EQ(r.accounted, 1u);
}

TEST(allocate_fee_preserves_total) {
    FeeAllocation a;
    auto r = AllocateFee(999, a);
    ASSERT_EQ(r.accounted, 999u);
}

TEST(treasury_balance_total_controlled) {
    TreasuryBalance b;
    b.security = 100;
    b.ecosystem = 50;
    b.developer = 30;
    b.reserve = 20;
    b.burned = 500;
    ASSERT_EQ(b.total_controlled(), 200u);
    ASSERT_EQ(b.total_all(), 700u);
}

TEST(treasury_ledger_record_fee) {
    TreasuryLedger ledger;
    ASSERT_TRUE(ledger.RecordNetworkFee(1000));
    const auto& b = ledger.balance();
    ASSERT_EQ(b.security, 500u);
    ASSERT_EQ(b.burned, 200u);
}

TEST(treasury_ledger_multiple_fees) {
    TreasuryLedger ledger;
    ledger.RecordNetworkFee(1000);
    ledger.RecordNetworkFee(2000);
    const auto& b = ledger.balance();
    ASSERT_EQ(b.security, 1500u);
    ASSERT_EQ(b.burned, 600u);
}

TEST(treasury_ledger_deposit) {
    TreasuryLedger ledger;
    ASSERT_TRUE(ledger.deposit("security", 500));
    ASSERT_EQ(ledger.balance().security, 500u);
}

TEST(treasury_ledger_deposit_all_categories) {
    TreasuryLedger ledger;
    ASSERT_TRUE(ledger.deposit("security", 100));
    ASSERT_TRUE(ledger.deposit("ecosystem", 200));
    ASSERT_TRUE(ledger.deposit("developer", 300));
    ASSERT_TRUE(ledger.deposit("reserve", 400));
    ASSERT_EQ(ledger.total_controlled(), 1000u);
}

TEST(treasury_ledger_deposit_invalid_category) {
    TreasuryLedger ledger;
    ASSERT_FALSE(ledger.deposit("invalid", 100));
}

TEST(treasury_ledger_deposit_zero_fails) {
    TreasuryLedger ledger;
    ASSERT_FALSE(ledger.deposit("security", 0));
}

TEST(treasury_ledger_withdraw) {
    TreasuryLedger ledger;
    ledger.deposit("security", 1000);
    ASSERT_TRUE(ledger.withdraw("security", 300));
    ASSERT_EQ(ledger.balance().security, 700u);
}

TEST(treasury_ledger_withdraw_insufficient) {
    TreasuryLedger ledger;
    ledger.deposit("security", 100);
    ASSERT_FALSE(ledger.withdraw("security", 500));
    ASSERT_EQ(ledger.balance().security, 100u);
}

TEST(treasury_ledger_reset) {
    TreasuryLedger ledger;
    ledger.RecordNetworkFee(1000);
    ASSERT_TRUE(ledger.total_controlled() > 0);
    ledger.reset();
    ASSERT_EQ(ledger.total_controlled(), 0u);
    ASSERT_EQ(ledger.total_burned(), 0u);
}

TEST(multisig_valid_config) {
    MultisigTreasury ms({"alice", "bob", "charlie"}, 2, 10);
    ASSERT_TRUE(ms.valid());
}

TEST(multisig_invalid_empty_signers) {
    MultisigTreasury ms({}, 1, 10);
    ASSERT_FALSE(ms.valid());
}

TEST(multisig_invalid_threshold_zero) {
    MultisigTreasury ms({"alice", "bob"}, 0, 10);
    ASSERT_FALSE(ms.valid());
}

TEST(multisig_invalid_threshold_too_high) {
    MultisigTreasury ms({"alice", "bob"}, 5, 10);
    ASSERT_FALSE(ms.valid());
}

TEST(multisig_invalid_duplicate_signers) {
    MultisigTreasury ms({"alice", "alice"}, 2, 10);
    ASSERT_FALSE(ms.valid());
}

TEST(timelock_ready_check) {
    Timelock t;
    t.delay_blocks = 10;
    t.queued_at_height = 100;
    ASSERT_FALSE(t.ready(100));
    ASSERT_FALSE(t.ready(105));
    ASSERT_TRUE(t.ready(110));
    ASSERT_TRUE(t.ready(115));
}

TEST(multisig_create_proposal) {
    MultisigTreasury ms({"alice", "bob", "charlie"}, 2, 10);
    auto p = ms.create_proposal("P1", "vendor", 5000, 100);
    ASSERT_EQ(p.id, std::string("P1"));
    ASSERT_EQ(p.destination, std::string("vendor"));
    ASSERT_EQ(p.amount, 5000u);
    ASSERT_FALSE(p.executed);
    ASSERT_FALSE(p.cancelled);
}

TEST(multisig_approve) {
    MultisigTreasury ms({"alice", "bob", "charlie"}, 2, 10);
    auto p = ms.create_proposal("P1", "vendor", 5000, 100);
    ASSERT_TRUE(ms.approve(p, "alice"));
    ASSERT_TRUE(ms.approve(p, "bob"));
    ASSERT_EQ(ms.approval_count(p), 2u);
}

TEST(multisig_approve_non_signer_fails) {
    MultisigTreasury ms({"alice", "bob"}, 2, 10);
    auto p = ms.create_proposal("P1", "vendor", 5000, 100);
    ASSERT_FALSE(ms.approve(p, "charlie"));
}

TEST(multisig_execute_before_timelock_fails) {
    MultisigTreasury ms({"alice", "bob"}, 2, 10);
    auto p = ms.create_proposal("P1", "vendor", 5000, 100);
    ms.approve(p, "alice");
    ms.approve(p, "bob");
    ASSERT_FALSE(ms.executable(p, 105));
    ASSERT_FALSE(ms.execute(p, 105));
}

TEST(multisig_execute_after_timelock) {
    MultisigTreasury ms({"alice", "bob"}, 2, 10);
    auto p = ms.create_proposal("P1", "vendor", 5000, 100);
    ms.approve(p, "alice");
    ms.approve(p, "bob");
    ASSERT_TRUE(ms.executable(p, 110));
    ASSERT_TRUE(ms.execute(p, 110));
    ASSERT_TRUE(p.executed);
}

TEST(multisig_insufficient_approvals) {
    MultisigTreasury ms({"alice", "bob", "charlie"}, 2, 10);
    auto p = ms.create_proposal("P1", "vendor", 5000, 100);
    ms.approve(p, "alice");  // Only 1 of 2
    ASSERT_FALSE(ms.executable(p, 110));
}

TEST(multisig_cancel) {
    MultisigTreasury ms({"alice", "bob"}, 2, 10);
    auto p = ms.create_proposal("P1", "vendor", 5000, 100);
    ASSERT_TRUE(ms.cancel(p, "alice"));
    ASSERT_TRUE(p.cancelled);
    ASSERT_FALSE(ms.execute(p, 110));
}

TEST(multisig_approve_after_execute_fails) {
    MultisigTreasury ms({"alice", "bob"}, 2, 10);
    auto p = ms.create_proposal("P1", "vendor", 5000, 100);
    ms.approve(p, "alice");
    ms.approve(p, "bob");
    ms.execute(p, 110);
    ASSERT_FALSE(ms.approve(p, "alice"));
}

int main() {
    return shivrai::test::TestRunner::instance().run();
}
