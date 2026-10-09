#include "test_framework.h"
#include "shivrai/fintech/payment.h"
using namespace shivrai::fintech;

TEST(send_payment_basic) {
    PaymentProcessor p(100);
    auto r = p.send("a", "b", 1000, "HON", "t");
    ASSERT_TRUE(r.success);
}

TEST(send_payment_multiple) {
    PaymentProcessor p(100);
    auto r1 = p.send("a", "b", 1000, "HON", "t1");
    auto r2 = p.send("c", "d", 2000, "HON", "t2");
    ASSERT_TRUE(r1.success);
    ASSERT_TRUE(r2.success);
}

TEST(create_invoice) {
    PaymentProcessor p(100);
    auto inv = p.create_invoice("m1", 5000, "HON", 3600, "memo");
    ASSERT_FALSE(inv.id.empty());
    ASSERT_EQ(inv.amount, 5000u);
}

TEST(pay_invoice_success) {
    PaymentProcessor p(100);
    auto inv = p.create_invoice("m1", 5000, "HON", 3600, "memo");
    auto r = p.pay_invoice(inv.id, "c1");
    ASSERT_TRUE(r.success);
}

TEST(pay_invoice_twice_fails) {
    PaymentProcessor p(100);
    auto inv = p.create_invoice("m1", 5000, "HON", 3600, "memo");
    p.pay_invoice(inv.id, "c1");
    auto r2 = p.pay_invoice(inv.id, "c1");
    ASSERT_FALSE(r2.success);
}

TEST(callback_on_status_change) {
    PaymentProcessor p(100);
    int count = 0;
    p.on_status_change([&](const Payment&) { count++; });
    p.send("a", "b", 1000, "HON", "callback-test");
    ASSERT_EQ(count, 1);
}

int main() {
    return shivrai::test::TestRunner::instance().run();
}
