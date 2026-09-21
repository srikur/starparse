#include "test_support.hpp"

#include <string>

using namespace StarParse;

namespace {
    struct Args {
        [[=Positional{0}]] std::string arg1;
        [[=Opt{'v', "Verbose mode"}]] bool verbose;
        [[=Positional{1}]] int arg2;
    };
}

TEST_CASE("positional: filled in order around options") {
    const auto args = parse_from<Args>({"hello", "-v", "42"});
    REQUIRE(args);
    CHECK(args->arg1 == "hello");
    CHECK(args->verbose);
    CHECK(args->arg2 == 42);
}

TEST_CASE("positional: also addressable by field name") {
    const auto args = parse_from<Args>({"--arg1", "hello", "--arg2", "42"});
    REQUIRE(args);
    CHECK(args->arg1 == "hello");
    CHECK_FALSE(args->verbose);
    CHECK(args->arg2 == 42);
}
