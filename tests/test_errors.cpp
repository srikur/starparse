#include "test_support.hpp"

using namespace StarParse;

namespace {
    struct Args {
        [[=Opt{'v', "Verbose mode"}]] bool verbose;
        [[=Positional{0}]] int arg1;
    };

    constexpr Settings settings{.allow_repeated_counts = false};
}

TEST_CASE("errors: unknown option") {
    const auto args = parse_from<Args>({"--nope"}, settings);
    REQUIRE_FALSE(args);
    REQUIRE(args.errors().size() == 1uz);
    CHECK(args.errors()[0].kind == ErrorKind::UNKNOWN_OPTION);
    CHECK(args.error_message() == "Unknown option 'nope'");
}

TEST_CASE("errors: option without value") {
    const auto args = parse_from<Args>({"--arg1"}, settings);
    REQUIRE_FALSE(args);
    REQUIRE(args.errors().size() == 1uz);
    CHECK(args.errors()[0].kind == ErrorKind::MISSING_VALUE);
    CHECK(args.error_message() == "Missing value for argument 'arg1'");
}

TEST_CASE("errors: value that does not parse") {
    const auto args = parse_from<Args>({"abc"}, settings);
    REQUIRE_FALSE(args);
    REQUIRE(args.errors().size() == 1uz);
    CHECK(args.errors()[0].kind == ErrorKind::INVALID_VALUE);
    CHECK(args.error_message() == "Could not parse input 'abc' for argument 'int'");
}
