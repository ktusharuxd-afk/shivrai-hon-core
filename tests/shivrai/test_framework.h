#ifndef SHIVRAI_TEST_FRAMEWORK_H
#define SHIVRAI_TEST_FRAMEWORK_H
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <sstream>
#include <stdexcept>

namespace shivrai::test {
struct TestCase { std::string name; std::function<void()> fn; };
class TestRunner {
public:
    static TestRunner& instance() { static TestRunner r; return r; }
    void add(const std::string& n, std::function<void()> f) { tests_.push_back({n, f}); }
    int run() {
        int p = 0, f = 0;
        std::cout << "\nRunning " << tests_.size() << " tests...\n";
        for (auto& t : tests_) {
            try { t.fn(); std::cout << "  PASS: " << t.name << "\n"; p++; }
            catch (const std::exception& e) {
                std::cout << "  FAIL: " << t.name << " -> " << e.what() << "\n"; f++;
            }
        }
        std::cout << "\nTotal: " << tests_.size() << "  Passed: " << p << "  Failed: " << f << "\n";
        return f == 0 ? 0 : 1;
    }
private:
    std::vector<TestCase> tests_;
};
struct TestRegistrar {
    TestRegistrar(const std::string& n, std::function<void()> f) {
        TestRunner::instance().add(n, f);
    }
};
}
#define TEST(name) \
    static void test_##name(); \
    static shivrai::test::TestRegistrar reg_##name(#name, test_##name); \
    static void test_##name()
#define ASSERT_TRUE(x) do { if (!(x)) throw std::runtime_error("ASSERT_TRUE: " #x); } while(0)
#define ASSERT_FALSE(x) do { if (x) throw std::runtime_error("ASSERT_FALSE: " #x); } while(0)
#define ASSERT_EQ(a,b) do { auto _a=(a); auto _b=(b); if (!(_a==_b)) { \
    std::ostringstream _ss; _ss << "ASSERT_EQ: " #a " vs " #b; \
    throw std::runtime_error(_ss.str()); } } while(0)
#define ASSERT_NE(a,b) do { if ((a) == (b)) throw std::runtime_error("ASSERT_NE: " #a " == " #b); } while(0)
#define ASSERT_NOT_NULL(x) do { if ((x)==nullptr) throw std::runtime_error("NULL: " #x); } while(0)
#endif
