#pragma once

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace mininginx::testing {

struct TestCase {
    std::string name;
    void (*func)();
};

inline std::vector<TestCase>& get_tests() {
    static std::vector<TestCase> tests;
    return tests;
}

struct TestRegistrar {
    TestRegistrar(const char* name, void (*func)()) {
        get_tests().push_back({name, func});
    }
};

#define TEST(suite, name) \
    static void test_##suite##_##name(); \
    static mininginx::testing::TestRegistrar reg_##suite##_##name(#suite "." #name, test_##suite##_##name); \
    static void test_##suite##_##name()

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: " #cond " at " << __FILE__ << ":" << __LINE__ << "\n"; \
            std::exit(1); \
        } \
    } while (0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            std::cerr << "Assertion failed: (" #a " == " #b ") [" << (a) << " != " << (b) << "] at " \
                      << __FILE__ << ":" << __LINE__ << "\n"; \
            std::exit(1); \
        } \
    } while (0)

#define ASSERT_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            std::cerr << "Assertion failed: (" #a " != " #b ") [" << (a) << " == " << (b) << "] at " \
                      << __FILE__ << ":" << __LINE__ << "\n"; \
            std::exit(1); \
        } \
    } while (0)

}  // namespace mininginx::testing
