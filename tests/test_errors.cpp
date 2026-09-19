#include <print>
#include <starparse/starparse.hpp>
#include <string>

struct Args {
    [[=StarParse::Opt{'v', "Verbose mode"}]] bool verbose;
    [[=StarParse::Positional{0}]] int arg1;
};

auto main(int argc, char **argv) -> int {
    try {
        const auto args{StarParse::parse_or_throw<Args>(argc, argv, StarParse::Settings{.allow_repeated_counts = false})};
        std::println("parsed verbose: {}, arg1: {}", args->verbose, args->arg1);
    } catch (const std::exception &e) {
        std::println("error: {}", e.what());
    }
}
