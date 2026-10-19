#include "tests/harness.hpp"
#include "core/validate.hpp"

using namespace aester::validate;

int main() {
    EXPECT_TRUE(is_valid_cpufreq_arg("40"));
    EXPECT_TRUE(is_valid_cpufreq_arg("70"));
    EXPECT_TRUE(is_valid_cpufreq_arg("90"));
    EXPECT_TRUE(is_valid_cpufreq_arg("off"));
    EXPECT_FALSE(is_valid_cpufreq_arg("100"));
    EXPECT_FALSE(is_valid_cpufreq_arg("35"));
    EXPECT_FALSE(is_valid_cpufreq_arg(""));
    EXPECT_FALSE(is_valid_cpufreq_arg("random"));

    EXPECT_TRUE(is_valid_identifier("schedutil"));
    EXPECT_TRUE(is_valid_identifier("mq-deadline"));
    EXPECT_TRUE(is_valid_identifier("performance_1"));
    EXPECT_FALSE(is_valid_identifier(""));
    EXPECT_FALSE(is_valid_identifier("sched;rm"));
    EXPECT_FALSE(is_valid_identifier("with space"));
    EXPECT_FALSE(is_valid_identifier("bad/path"));

    std::vector<std::string> l1 = {"schedutil", "performance", "powersave"};
    std::vector<std::string> l2 = {"schedutil", "performance", "ondemand"};
    std::vector<std::string> l3 = {"schedutil", "powersave"};

    auto r1 = intersect_lists({l1, l2});
    EXPECT_EQ(r1.size(), 2u);
    EXPECT_TRUE(is_in_list("schedutil", r1));
    EXPECT_TRUE(is_in_list("performance", r1));
    EXPECT_FALSE(is_in_list("powersave", r1));

    auto r2 = intersect_lists({l1, l2, l3});
    EXPECT_EQ(r2.size(), 1u);
    EXPECT_TRUE(is_in_list("schedutil", r2));

    return aester::test::report();
}
