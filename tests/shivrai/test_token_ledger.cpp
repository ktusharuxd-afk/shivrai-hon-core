#include "test_framework.h"
#include "shivrai/assets/token_ledger.h"

using namespace shivrai::assets;

TEST(token_metadata) {
    TokenLedger t("HON", 8, 1000000);
    ASSERT_EQ(t.symbol(), std::string("HON"));
    ASSERT_EQ(t.decimals(), 8);
    ASSERT_EQ(t.max_supply(), 1000000u);
    ASSERT_EQ(t.total_supply(), 0u);
}

TEST(token_mint_basic) {
    TokenLedger t("HON");
    ASSERT_TRUE(t.mint("alice", 1000));
    ASSERT_EQ(t.balance("alice"), 1000u);
    ASSERT_EQ(t.total_supply(), 1000u);
}

TEST(token_mint_zero_fails) {
    TokenLedger t("HON");
    ASSERT_FALSE(t.mint("alice", 0));
}

TEST(token_mint_respects_max_supply) {
    TokenLedger t("HON", 18, 1000);
    ASSERT_TRUE(t.mint("alice", 1000));
    ASSERT_FALSE(t.mint("bob", 1));
    ASSERT_EQ(t.total_supply(), 1000u);
}

TEST(token_mint_multiple_accounts) {
    TokenLedger t("HON");
    t.mint("alice", 100);
    t.mint("bob", 200);
    t.mint("charlie", 300);
    ASSERT_EQ(t.balance("alice"), 100u);
    ASSERT_EQ(t.balance("bob"), 200u);
    ASSERT_EQ(t.balance("charlie"), 300u);
    ASSERT_EQ(t.total_supply(), 600u);
    ASSERT_EQ(t.account_count(), 3u);
}

TEST(token_burn_basic) {
    TokenLedger t("HON");
    t.mint("alice", 1000);
    ASSERT_TRUE(t.burn("alice", 300));
    ASSERT_EQ(t.balance("alice"), 700u);
    ASSERT_EQ(t.total_supply(), 700u);
}

TEST(token_burn_insufficient_fails) {
    TokenLedger t("HON");
    t.mint("alice", 100);
    ASSERT_FALSE(t.burn("alice", 500));
    ASSERT_EQ(t.balance("alice"), 100u);
}

TEST(token_burn_unknown_account_fails) {
    TokenLedger t("HON");
    ASSERT_FALSE(t.burn("nobody", 10));
}

TEST(token_burn_zero_fails) {
    TokenLedger t("HON");
    t.mint("alice", 100);
    ASSERT_FALSE(t.burn("alice", 0));
}

TEST(token_transfer_basic) {
    TokenLedger t("HON");
    t.mint("alice", 1000);
    ASSERT_TRUE(t.transfer("alice", "bob", 300));
    ASSERT_EQ(t.balance("alice"), 700u);
    ASSERT_EQ(t.balance("bob"), 300u);
    ASSERT_EQ(t.total_supply(), 1000u);
}

TEST(token_transfer_to_self_fails) {
    TokenLedger t("HON");
    t.mint("alice", 1000);
    ASSERT_FALSE(t.transfer("alice", "alice", 100));
}

TEST(token_transfer_insufficient_fails) {
    TokenLedger t("HON");
    t.mint("alice", 100);
    ASSERT_FALSE(t.transfer("alice", "bob", 500));
    ASSERT_EQ(t.balance("alice"), 100u);
    ASSERT_EQ(t.balance("bob"), 0u);
}

TEST(token_transfer_zero_fails) {
    TokenLedger t("HON");
    t.mint("alice", 100);
    ASSERT_FALSE(t.transfer("alice", "bob", 0));
}

TEST(token_transfer_exact_balance) {
    TokenLedger t("HON");
    t.mint("alice", 500);
    ASSERT_TRUE(t.transfer("alice", "bob", 500));
    ASSERT_EQ(t.balance("alice"), 0u);
    ASSERT_EQ(t.balance("bob"), 500u);
}

TEST(token_approve_and_allowance) {
    TokenLedger t("HON");
    ASSERT_TRUE(t.approve("alice", "spender", 500));
    ASSERT_EQ(t.allowance("alice", "spender"), 500u);
}

TEST(token_allowance_default_zero) {
    TokenLedger t("HON");
    ASSERT_EQ(t.allowance("alice", "spender"), 0u);
}

TEST(token_increase_allowance) {
    TokenLedger t("HON");
    t.approve("alice", "spender", 100);
    ASSERT_TRUE(t.increase_allowance("alice", "spender", 50));
    ASSERT_EQ(t.allowance("alice", "spender"), 150u);
}

TEST(token_decrease_allowance) {
    TokenLedger t("HON");
    t.approve("alice", "spender", 100);
    ASSERT_TRUE(t.decrease_allowance("alice", "spender", 30));
    ASSERT_EQ(t.allowance("alice", "spender"), 70u);
}

TEST(token_decrease_allowance_below_zero_fails) {
    TokenLedger t("HON");
    t.approve("alice", "spender", 100);
    ASSERT_FALSE(t.decrease_allowance("alice", "spender", 200));
    ASSERT_EQ(t.allowance("alice", "spender"), 100u);
}

TEST(token_transfer_from_success) {
    TokenLedger t("HON");
    t.mint("alice", 1000);
    t.approve("alice", "spender", 500);
    ASSERT_TRUE(t.transfer_from("spender", "alice", "bob", 300));
    ASSERT_EQ(t.balance("alice"), 700u);
    ASSERT_EQ(t.balance("bob"), 300u);
    ASSERT_EQ(t.allowance("alice", "spender"), 200u);
}

TEST(token_transfer_from_insufficient_allowance) {
    TokenLedger t("HON");
    t.mint("alice", 1000);
    t.approve("alice", "spender", 100);
    ASSERT_FALSE(t.transfer_from("spender", "alice", "bob", 500));
    ASSERT_EQ(t.balance("alice"), 1000u);
    ASSERT_EQ(t.balance("bob"), 0u);
}

TEST(token_freeze_blocks_transfer) {
    TokenLedger t("HON");
    t.mint("alice", 1000);
    t.freeze("alice");
    ASSERT_TRUE(t.is_frozen("alice"));
    ASSERT_FALSE(t.transfer("alice", "bob", 100));
}

TEST(token_freeze_blocks_mint) {
    TokenLedger t("HON");
    t.freeze("alice");
    ASSERT_FALSE(t.mint("alice", 100));
}

TEST(token_unfreeze_restores) {
    TokenLedger t("HON");
    t.mint("alice", 1000);
    t.freeze("alice");
    ASSERT_FALSE(t.transfer("alice", "bob", 100));
    t.unfreeze("alice");
    ASSERT_FALSE(t.is_frozen("alice"));
    ASSERT_TRUE(t.transfer("alice", "bob", 100));
}

TEST(token_frozen_recipient_blocks) {
    TokenLedger t("HON");
    t.mint("alice", 1000);
    t.freeze("bob");
    ASSERT_FALSE(t.transfer("alice", "bob", 100));
}

int main() {
    return shivrai::test::TestRunner::instance().run();
}
