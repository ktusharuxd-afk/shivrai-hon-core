# Shivrai Test Suite

Lightweight unit tests for SHIVRAI HON modules.

## Overview

| Suite | Tests | Coverage |
|-------|-------|----------|
| test_identity | 8 | DID, KYC, credentials |
| test_interop | 7 | Cross-chain relay |
| test_rwa | 6 | Asset lifecycle |
| test_gaming | 7 | Game assets |
| test_payment | 6 | Payments, invoices |
| **Total** | **34** | **All passing** |

## Quick Start

    cd tests/shivrai
    cmake -B build -S .
    cmake --build build -j 4
    cd build

    # Run individual suites
    ./test_identity
    ./test_interop
    ./test_rwa
    ./test_gaming
    ./test_payment

    # Or use CTest
    ctest --output-on-failure

## Framework

Header-only test framework in `test_framework.h`:

    #include "test_framework.h"

    TEST(my_test_name) {
        ASSERT_TRUE(condition);
        ASSERT_FALSE(condition);
        ASSERT_EQ(a, b);
        ASSERT_NE(a, b);
        ASSERT_NOT_NULL(ptr);
    }

    // At end of file:
    int main() {
        return shivrai::test::TestRunner::instance().run();
    }

## Adding a New Test

1. Create `test_mymodule.cpp`
2. Include `test_framework.h` and your module header
3. Write TEST blocks
4. Add `main()` at end (calls `TestRunner::instance().run()`)
5. Register in `CMakeLists.txt`:
   - Add to `SHIVRAI_SOURCES` if new source file
   - Add executable line: `add_executable(test_mymodule ...)`
   - Add test: `add_test(NAME test_mymodule COMMAND test_mymodule)`

## CI

Every push triggers GitHub Actions:
- Workflow: `.github/workflows/shivrai-tests.yml`
- Builds all test suites on Ubuntu
- Runs all 34 tests
- Fails if any test fails

## Design Notes

**Why separate from Bitcoin Core?**
- Bitcoin Core has its own heavy test suite
- Shivrai modules are non-consensus — safer to test in isolation
- Fast iteration (~5 sec build vs Bitcoin's ~30 min)

**Why `std::recursive_mutex`?**
- Methods like `is_active()` call `resolve()` internally
- Plain mutex would deadlock on same-thread re-lock
- Recursive allows safe re-entry

**Why callbacks fire outside lock?**
- Prevents deadlock when callback re-enters registry
- Copies callback pointer under lock, invokes after release
