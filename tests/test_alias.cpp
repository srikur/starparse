#include "test_support.hpp"

#include <string>

using namespace StarParse;

namespace {
    struct Args {
        [[=Positional{0}, =Alias{"source"}]] std::string input;
        [[=Opt{'v', "Verbose mode"}, =Alias{"talkative", "q"}]] bool verbose;
        [[=Opt{'k', "Keep temporary files"}]] bool keep;
        [[=Positional{1}, =Alias{"count"}]] int arg2;
    };
}

TEST_CASE("alias: long alias for an option") {
    const auto args = parse_from<Args>({"--talkative", "hello", "42"});
    REQUIRE(args);
    CHECK(args->input == "hello");
    CHECK(args->verbose);
    CHECK_FALSE(args->keep);
    CHECK(args->arg2 == 42);
}

TEST_CASE("alias: positionals addressable by alias") {
    const auto args = parse_from<Args>({"--source=hello", "--count=42"});
    REQUIRE(args);
    CHECK(args->input == "hello");
    CHECK_FALSE(args->verbose);
    CHECK(args->arg2 == 42);
}

TEST_CASE("alias: single-character alias") {
    const auto args = parse_from<Args>({"-q", "hello", "42"});
    REQUIRE(args);
    CHECK(args->input == "hello");
    CHECK(args->verbose);
    CHECK_FALSE(args->keep);
    CHECK(args->arg2 == 42);
}

TEST_CASE("alias: single-character alias inside a bundle") {
    const auto args = parse_from<Args>({"-qk", "hello", "42"});
    REQUIRE(args);
    CHECK(args->input == "hello");
    CHECK(args->verbose);
    CHECK(args->keep);
    CHECK(args->arg2 == 42);
}
