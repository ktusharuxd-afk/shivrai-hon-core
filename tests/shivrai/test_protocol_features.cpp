#include "test_framework.h"
#include "shivrai/protocol_features.h"

using namespace shivrai;

TEST(builtin_features_not_empty) {
    const auto& f = GlobalProtocolFeatures();
    ASSERT_TRUE(f.size() > 0);
}

TEST(builtin_features_has_identity) {
    const auto& f = GlobalProtocolFeatures();
    bool found = false;
    for (const auto& x : f) {
        if (std::string(x.id) == "identity") found = true;
    }
    ASSERT_TRUE(found);
}

TEST(builtin_features_has_amm) {
    const auto& f = GlobalProtocolFeatures();
    bool found = false;
    for (const auto& x : f) {
        if (std::string(x.id) == "amm") found = true;
    }
    ASSERT_TRUE(found);
}

TEST(builtin_features_has_treasury) {
    const auto& f = GlobalProtocolFeatures();
    bool found = false;
    for (const auto& x : f) {
        if (std::string(x.id) == "treasury") found = true;
    }
    ASSERT_TRUE(found);
}

TEST(feature_status_name_foundation) {
    ASSERT_EQ(std::string(FeatureStatusName(FeatureStatus::FOUNDATION)),
              std::string("foundation"));
}

TEST(feature_status_name_planned) {
    ASSERT_EQ(std::string(FeatureStatusName(FeatureStatus::PLANNED)),
              std::string("planned"));
}

TEST(feature_status_name_active) {
    ASSERT_EQ(std::string(FeatureStatusName(FeatureStatus::ACTIVE)),
              std::string("active"));
}

TEST(feature_status_name_deprecated) {
    ASSERT_EQ(std::string(FeatureStatusName(FeatureStatus::DEPRECATED)),
              std::string("deprecated"));
}

TEST(registry_initially_empty) {
    FeatureRegistry reg;
    ASSERT_EQ(reg.size(), 0u);
    ASSERT_EQ(reg.active_count(), 0u);
}

TEST(registry_register_basic) {
    FeatureRegistry reg;
    ASSERT_TRUE(reg.register_feature("test", "Test Feature", "testing"));
    ASSERT_EQ(reg.size(), 1u);
    ASSERT_TRUE(reg.has_feature("test"));
}

TEST(registry_register_duplicate_fails) {
    FeatureRegistry reg;
    reg.register_feature("test", "Test", "testing");
    ASSERT_FALSE(reg.register_feature("test", "Test Again", "testing"));
    ASSERT_EQ(reg.size(), 1u);
}

TEST(registry_register_empty_id_fails) {
    FeatureRegistry reg;
    ASSERT_FALSE(reg.register_feature("", "Name", "domain"));
}

TEST(registry_register_empty_name_fails) {
    FeatureRegistry reg;
    ASSERT_FALSE(reg.register_feature("id", "", "domain"));
}

TEST(registry_activate) {
    FeatureRegistry reg;
    reg.register_feature("test", "Test", "testing");
    ASSERT_FALSE(reg.is_active("test"));
    ASSERT_TRUE(reg.activate("test", 100));
    ASSERT_TRUE(reg.is_active("test"));
}

TEST(registry_activate_twice_fails) {
    FeatureRegistry reg;
    reg.register_feature("test", "Test", "testing");
    reg.activate("test", 100);
    ASSERT_FALSE(reg.activate("test", 200));
}

TEST(registry_activate_unknown_fails) {
    FeatureRegistry reg;
    ASSERT_FALSE(reg.activate("nonexistent", 100));
}

TEST(registry_deactivate) {
    FeatureRegistry reg;
    reg.register_feature("test", "Test", "testing");
    reg.activate("test", 100);
    ASSERT_TRUE(reg.deactivate("test"));
    ASSERT_FALSE(reg.is_active("test"));
}

TEST(registry_deactivate_inactive_fails) {
    FeatureRegistry reg;
    reg.register_feature("test", "Test", "testing");
    ASSERT_FALSE(reg.deactivate("test"));
}

TEST(registry_find) {
    FeatureRegistry reg;
    reg.register_feature("test", "Test Feature", "testing");
    const auto* s = reg.find("test");
    ASSERT_NOT_NULL(s);
    ASSERT_EQ(s->id, std::string("test"));
    ASSERT_EQ(s->name, std::string("Test Feature"));
}

TEST(registry_find_unknown_returns_null) {
    FeatureRegistry reg;
    ASSERT_NULL(reg.find("nonexistent"));
}

TEST(registry_activation_height) {
    FeatureRegistry reg;
    reg.register_feature("test", "Test", "testing");
    reg.activate("test", 12345);
    const auto* s = reg.find("test");
    ASSERT_NOT_NULL(s);
    ASSERT_EQ(s->activation_height, 12345u);
}

TEST(registry_active_count) {
    FeatureRegistry reg;
    reg.register_feature("a", "A", "d");
    reg.register_feature("b", "B", "d");
    reg.register_feature("c", "C", "d");
    reg.activate("a");
    reg.activate("b");
    ASSERT_EQ(reg.active_count(), 2u);
}

TEST(registry_by_domain) {
    FeatureRegistry reg;
    reg.register_feature("a", "A", "defi");
    reg.register_feature("b", "B", "defi");
    reg.register_feature("c", "C", "tooling");
    auto defi = reg.by_domain("defi");
    ASSERT_EQ(defi.size(), 2u);
}

TEST(registry_list) {
    FeatureRegistry reg;
    reg.register_feature("a", "A", "d");
    reg.register_feature("b", "B", "d");
    auto list = reg.list();
    ASSERT_EQ(list.size(), 2u);
}

TEST(registry_register_with_active_status) {
    FeatureRegistry reg;
    reg.register_feature("test", "Test", "testing", FeatureStatus::ACTIVE);
    ASSERT_TRUE(reg.is_active("test"));
    ASSERT_EQ(reg.active_count(), 1u);
}

int main() {
    return shivrai::test::TestRunner::instance().run();
}
