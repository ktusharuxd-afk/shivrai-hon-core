#include "test_framework.h"
#include "shivrai/interop/message.h"
using namespace shivrai::interop;

TEST(send_message_basic) {
    MessageRelay r(2);
    auto id = r.send_message(ChainId::SHIVRAI, ChainId::ETHEREUM, "a", "b", "hi");
    ASSERT_FALSE(id.empty());
    ASSERT_EQ(r.status(id), MessageStatus::PENDING);
}

TEST(send_message_empty_fails) {
    MessageRelay r(2);
    ASSERT_TRUE(r.send_message(ChainId::SHIVRAI, ChainId::ETHEREUM, "", "b", "hi").empty());
}

TEST(multi_relayer_consensus) {
    MessageRelay r(2);
    auto id = r.send_message(ChainId::SHIVRAI, ChainId::ETHEREUM, "a", "b", "hi");
    r.relay_message(id, "r1", "s1");
    ASSERT_EQ(r.status(id), MessageStatus::PENDING);
    r.relay_message(id, "r2", "s2");
    ASSERT_EQ(r.status(id), MessageStatus::RELAYED);
}

TEST(confirm_message) {
    MessageRelay r(2);
    auto id = r.send_message(ChainId::SHIVRAI, ChainId::ETHEREUM, "a", "b", "hi");
    ASSERT_TRUE(r.confirm_message(id));
    ASSERT_EQ(r.status(id), MessageStatus::CONFIRMED);
}

TEST(fail_message) {
    MessageRelay r(2);
    auto id = r.send_message(ChainId::SHIVRAI, ChainId::ETHEREUM, "a", "b", "hi");
    ASSERT_TRUE(r.fail_message(id));
    ASSERT_EQ(r.status(id), MessageStatus::FAILED);
}

TEST(pending_messages_filter) {
    MessageRelay r(2);
    r.send_message(ChainId::SHIVRAI, ChainId::ETHEREUM, "a", "b", "p1");
    r.send_message(ChainId::SHIVRAI, ChainId::BNB, "a", "b", "p2");
    ASSERT_EQ(r.pending_messages(ChainId::ETHEREUM).size(), 1u);
}

TEST(callback_on_confirm) {
    MessageRelay r(2);
    int count = 0;
    r.on_confirmed([&](const CrossChainMessage&) { count++; });
    auto id = r.send_message(ChainId::SHIVRAI, ChainId::ETHEREUM, "a", "b", "hi");
    r.confirm_message(id);
    ASSERT_EQ(count, 1);
}

int main() {
    return shivrai::test::TestRunner::instance().run();
}
