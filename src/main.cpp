#include "cli/args.hpp"
#include "cli/commands.hpp"
#include "core/log.hpp"
#include "core/result.hpp"

int main(int argc, char* argv[]) {
    aester::log::init_log();
    aester::cli::ParsedArgs args = aester::cli::parse_args(argc, argv);
    return aester::cli::execute_command(args);
}
