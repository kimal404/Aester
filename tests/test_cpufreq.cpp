#include "harness.hpp"
#include "features/cpufreq/cpufreq.hpp"

using namespace aester::cpufreq;

int main() {
    std::string mock_freqs = "1600000 800000 2000000 1200000";
    auto parsed = parse_frequencies(mock_freqs);
    EXPECT_EQ(parsed.size(), 4u);
    EXPECT_EQ(parsed[0], 800000UL);
    EXPECT_EQ(parsed[3], 2000000UL);

    EXPECT_EQ(calculate_target_freq(2000000, 70), 1400000UL);
    EXPECT_EQ(calculate_target_freq(3250000, 50), 1625000UL);
    EXPECT_EQ(calculate_target_freq(2000000, 0), 0UL);

    EXPECT_EQ(find_closest_freq(parsed, 1400000), 1200000UL);
    EXPECT_EQ(find_closest_freq(parsed, 1600000), 1600000UL);
    EXPECT_EQ(find_closest_freq(parsed, 2500000), 2000000UL);
    EXPECT_EQ(find_closest_freq(parsed, 500000), 800000UL);

    return aester::test::report();
}
