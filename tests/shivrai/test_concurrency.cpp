#include "test_framework.h"
#include "shivrai/identity/identity.h"
#include "shivrai/interop/message.h"
#include "shivrai/assets/rwa.h"
#include "shivrai/gaming/assets.h"
#include "shivrai/fintech/payment.h"

#include <thread>
#include <atomic>
#include <vector>

using namespace shivrai;

// Helper: spawn N threads, each runs fn M times
template<typename Fn>
void parallel_run(int threads, int iters, Fn fn) {
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; t++) {
        pool.emplace_back([&, t]() {
            for (int i = 0; i < iters; i++) {
                fn(t, i);
            }
        });
    }
    for (auto& th : pool) th.join();
}

TEST(identity_concurrent_registration) {
    identity::IdentityRegistry reg;
    std::atomic<int> success{0};

    parallel_run(8, 100, [&](int t, int i) {
        std::string id = "did:u:" + std::to_string(t) + ":" + std::to_string(i);
        if (reg.register_did(id, "ctrl", "0xABC")) {
            success++;
        }
    });

    ASSERT_EQ(success.load(), 800);
    ASSERT_EQ(reg.did_count(), 800u);
}

TEST(identity_concurrent_reads) {
    identity::IdentityRegistry reg;
    reg.register_did("did:shared", "ctrl", "0xABC");

    std::atomic<int> active_count{0};
    parallel_run(8, 500, [&](int, int) {
        if (reg.is_active("did:shared")) {
            active_count++;
        }
    });

    ASSERT_EQ(active_count.load(), 4000);
}

TEST(interop_concurrent_sends) {
    interop::MessageRelay relay(2);
    std::atomic<int> success{0};

    parallel_run(8, 100, [&](int t, int i) {
        auto id = relay.send_message(
            interop::ChainId::SHIVRAI,
            interop::ChainId::ETHEREUM,
            "sender" + std::to_string(t),
            "receiver" + std::to_string(i),
            "payload"
        );
        if (!id.empty()) success++;
    });

    ASSERT_EQ(success.load(), 800);
    ASSERT_EQ(relay.message_count(), 800u);
}

TEST(rwa_concurrent_registrations) {
    assets::RWARegistry reg;
    std::atomic<int> success{0};

    parallel_run(8, 50, [&](int t, int i) {
        auto id = reg.register_asset(
            "owner" + std::to_string(t),
            assets::RWAType::INVOICE,
            "invoice" + std::to_string(i),
            1000,
            "USD", "issuer", "IN"
        );
        if (!id.empty()) success++;
    });

    ASSERT_EQ(success.load(), 400);
    ASSERT_EQ(reg.count(), 400u);
}

TEST(gaming_concurrent_mints) {
    gaming::GameAssetRegistry reg;
    std::atomic<int> success{0};

    parallel_run(8, 50, [&](int t, int i) {
        auto id = reg.mint_asset(
            "player" + std::to_string(t),
            "game1",
            "sword",
            "Excalibur" + std::to_string(i),
            gaming::AssetRarity::RARE
        );
        if (!id.empty()) success++;
    });

    ASSERT_EQ(success.load(), 400);
    ASSERT_EQ(reg.total_assets(), 400u);
}

TEST(payment_concurrent_sends) {
    fintech::PaymentProcessor proc(100);
    std::atomic<int> success{0};

    parallel_run(8, 50, [&](int t, int i) {
        auto r = proc.send(
            "sender" + std::to_string(t),
            "receiver" + std::to_string(i),
            1000, "HON", "test"
        );
        if (r.success) success++;
    });

    ASSERT_EQ(success.load(), 400);
}

int main() {
    return shivrai::test::TestRunner::instance().run();
}
