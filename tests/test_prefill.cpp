#include "test_support.hpp"

#include <string>

using namespace StarParse;

namespace {
    struct Flags {
        [[=Positional{0}]] std::string arg1;
        [[=Opt{'k', "Keep going"}]] bool keep;
        [[=Opt{'v', "Verbose mode"}]] bool verbose;
        [[=Positional{1}]] int arg2;
    };

    Flags defaults() {
        return {.arg1 = "default1", .keep = true, .verbose = false, .arg2 = 99};
    }
}

TEST_CASE("prefill: defaults survive when nothing is passed") {
    const auto args = parse_from<Flags>({}, defaults());
    REQUIRE(args);
    CHECK(args->arg1 == "default1");
    CHECK(args->keep);
    CHECK_FALSE(args->verbose);
    CHECK(args->arg2 == 99);
}

TEST_CASE("prefill: only mentioned fields are overwritten") {
    const auto args = parse_from<Flags>({"--arg2=5", "-v"}, defaults());
    REQUIRE(args);
    CHECK(args->arg1 == "default1");
    CHECK(args->keep);
    CHECK(args->verbose);
    CHECK(args->arg2 == 5);
}

TEST_CASE("prefill: positionals overwrite in order") {
    const auto args = parse_from<Flags>({"hello"}, defaults());
    REQUIRE(args);
    CHECK(args->arg1 == "hello");
    CHECK(args->keep);
    CHECK_FALSE(args->verbose);
    CHECK(args->arg2 == 99);
}
