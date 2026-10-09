#include "test_support.hpp"

#include <optional>
#include <string>
#include <vector>

using namespace StarParse;

namespace {
    struct Args {
        [[=Opt{'V', "Verbose mode"}]] bool verbose{false};
        [[=Opt{'o', "Output file"}]] std::string output;
        [[=Opt{'c', "Colorize"}, =Alias{"colour"}]] bool color{true};
        [[=Opt{'j', "Number of jobs"}]] int jobs{0};
        [[=Positional{0}]] std::string file;
    };

    constexpr Settings first_wins{.duplicate_option_policy = DuplicateOptionPolicy::FIRST_WINS};
    constexpr Settings last_wins{.duplicate_option_policy = DuplicateOptionPolicy::LAST_WINS};
    constexpr Settings errors{.duplicate_option_policy = DuplicateOptionPolicy::ERROR};
} // namespace

TEST_CASE("duplicate policy: with no override, last wins") {
    const auto args = parse_from<Args>({"--jobs", "6", "--jobs=4"});
    REQUIRE(args);
    CHECK(args->verbose == 4);
}

TEST_CASE("duplicate policy: explicit last wins passed") {
    const auto spaced = parse_from<Args>({"--out", "result.bin", "in.txt"}, last_wins);
    REQUIRE(args);
    CHECK(args->verbose == 4);
}
