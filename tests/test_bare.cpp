#include "test_support.hpp"

#include <string>

using namespace StarParse;

namespace {
    struct Bare {
        std::string arg1;
        bool verbose;
        int arg2;
    };
}

TEST_CASE("bare: fields are addressable by name with one or two dashes") {
    const auto args = parse_from<Bare>({"--arg1=hello", "-arg2=42"});
    REQUIRE(args);
    CHECK(args->arg1 == "hello");
    CHECK_FALSE(args->verbose);
    CHECK(args->arg2 == 42);
}

TEST_CASE("bare: non-bool fields are positional in declaration order") {
    const auto args = parse_from<Bare>({"hello", "42"});
    REQUIRE(args);
    CHECK(args->arg1 == "hello");
    CHECK_FALSE(args->verbose);
    CHECK(args->arg2 == 42);
}

TEST_CASE("bare: bool fields become flags") {
    const auto args = parse_from<Bare>({"hello", "--verbose", "42"});
    REQUIRE(args);
    CHECK(args->arg1 == "hello");
    CHECK(args->verbose);
    CHECK(args->arg2 == 42);
}
