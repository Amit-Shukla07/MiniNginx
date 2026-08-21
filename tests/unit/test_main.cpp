#include "test_framework.hpp"

#include <iostream>

int main() {
    auto& tests = mininginx::testing::get_tests();
    std::cout << "======================================\n";
    std::cout << "Running " << tests.size() << " MiniNginx Unit Tests\n";
    std::cout << "======================================\n";

    size_t passed = 0;
    for (const auto& t : tests) {
        std::cout << "[ RUN      ] " << t.name << "\n";
        t.func();
        std::cout << "[       OK ] " << t.name << "\n";
        passed++;
    }

    std::cout << "======================================\n";
    std::cout << "Test Summary: " << passed << "/" << tests.size() << " tests passed (100%).\n";
    std::cout << "======================================\n";
    return 0;
}
