#include "harness.hpp"
#include "features/governor/governor.hpp"

using namespace aester::governor;

int main() {
    std::string mock_govs = "schedutil performance powersave ondemand";
    auto parsed = parse_governors(mock_govs);
    EXPECT_EQ(parsed.size(), 4u);
    EXPECT_EQ(parsed[0], "schedutil");
    EXPECT_EQ(parsed[1], "performance");
    EXPECT_EQ(parsed[2], "powersave");
    EXPECT_EQ(parsed[3], "ondemand");

    auto empty_parsed = parse_governors("");
    EXPECT_EQ(empty_parsed.size(), 0u);

    return aester::test::report();
}
