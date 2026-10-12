#include "test_support.hpp"

#include <string>

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
    CHECK(args->jobs == 4);
}

TEST_CASE("duplicate policy: explicit last wins passed") {
    const auto args = parse_from<Args, last_wins>({"--jobs", "6", "--jobs", "9"});
    REQUIRE(args);
    CHECK(args.errors().size() == 0);
    CHECK(args->jobs == 9);
}

TEST_CASE("duplicate policy: explicit first wins passed") {
    const auto args = parse_from<Args, first_wins>({"--jobs", "6", "--jobs", "9"});
    REQUIRE(args);
    CHECK(args.errors().size() == 0);
    CHECK(args->jobs == 6);
}

TEST_CASE("duplicate policy: explicit error on duplicate") {
    const auto args = parse_from<Args, errors>({"--jobs", "6", "--jobs", "9"});
    REQUIRE_FALSE(args);
    CHECK(args.errors().size() == 1);
    CHECK(args.errors()[0].kind == ErrorKind::DUPLICATE_OPTION);
    CHECK(args->jobs == 6);
}
