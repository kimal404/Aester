#pragma once

#include <iostream>
#include <string>

namespace aester {
namespace test {

inline int run_count = 0;
inline int fail_count = 0;

inline void check(bool cond, const std::string& name, const char* file, int line) {
    ++run_count;
    if (!cond) {
        ++fail_count;
        std::cerr << "FAILED: " << name << " (" << file << ":" << line << ")\n";
    } else {
        std::cout << "PASSED: " << name << "\n";
    }
}

#define EXPECT_TRUE(cond) aester::test::check((cond), #cond, __FILE__, __LINE__)
#define EXPECT_FALSE(cond) aester::test::check(!(cond), "!" #cond, __FILE__, __LINE__)
#define EXPECT_EQ(a, b) aester::test::check((a) == (b), #a " == " #b, __FILE__, __LINE__)

inline int report() {
    std::cout << "\n=== Test Results: "
              << (run_count - fail_count) << "/" << run_count
              << " passed";
    if (fail_count > 0) {
        std::cout << " (" << fail_count << " FAILED)";
    }
    std::cout << " ===\n";
    return fail_count == 0 ? 0 : 1;
}

} // namespace test
} // namespace aester
