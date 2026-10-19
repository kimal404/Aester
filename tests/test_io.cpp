#include "harness.hpp"
#include "features/io/io.hpp"

using namespace aester::io;

int main() {
    std::string mock_sched = "none [mq-deadline] kyber bfq";
    auto parsed = parse_scheduler_line(mock_sched);

    EXPECT_EQ(parsed.current, "mq-deadline");
    EXPECT_EQ(parsed.available.size(), 4u);
    EXPECT_EQ(parsed.available[0], "none");
    EXPECT_EQ(parsed.available[1], "mq-deadline");
    EXPECT_EQ(parsed.available[2], "kyber");
    EXPECT_EQ(parsed.available[3], "bfq");

    std::string single_sched = "[none]";
    auto parsed_single = parse_scheduler_line(single_sched);
    EXPECT_EQ(parsed_single.current, "none");
    EXPECT_EQ(parsed_single.available.size(), 1u);
    EXPECT_EQ(parsed_single.available[0], "none");

    return aester::test::report();
}
