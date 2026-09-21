#include "test_support.hpp"

#include <array>
#include <stdexcept>

using namespace StarParse;

namespace {
    struct Args {
        [[=Opt{'v', "Verbose mode"}]] bool verbose;
        [[=Opt{'a', "Argument 1 help string"}]] int arg1;
    };
}

TEST_CASE("entry points: parse(argc argv) skips argv[0]") {
    constexpr std::array argv{"program", "-a", "3", "--verbose"};
    const auto args = parse<Args>(argv.size(), argv.data());
    REQUIRE(args);
    CHECK(args->verbose);
    CHECK(args->arg1 == 3);
}

TEST_CASE("entry points: parse(argc argv) with only argv[0] parses nothing") {
    constexpr std::array argv{"program"};
    const auto args = parse<Args>(argv.size(), argv.data());
    REQUIRE(args);
    CHECK_FALSE(args->verbose);
    CHECK(args->arg1 == 0);
}

TEST_CASE("entry points: parse() reports errors instead of throwing") {
    constexpr std::array argv{"program", "--fake_arg"};
    const auto args = parse<Args>(argv.size(), argv.data());
    REQUIRE_FALSE(args);
    CHECK(args.error_message() == "Unknown option 'fake_arg'");
}

TEST_CASE("entry points: parse_or_throw() throws the first error as std::invalid_argument") {
    constexpr std::array bad{"program", "--nope", "--also-nope"};
    CHECK_THROWS_WITH_AS(parse_or_throw<Args>(bad.size(), bad.data()),
                         "Unknown option 'nope'", std::invalid_argument);

    constexpr std::array good{"program", "-a", "7"};
    const auto args = parse_or_throw<Args>(good.size(), good.data());
    CHECK(args->arg1 == 7);
}

TEST_CASE("entry points: parse_from() matches parse(argc argv)") {
    constexpr std::array argv{"program", "-a", "3", "--verbose"};
    const auto from_argv = parse<Args>(argv.size(), argv.data());
    const auto from_list = parse_from<Args>({"-a", "3", "--verbose"});
    REQUIRE(from_argv);
    REQUIRE(from_list);
    CHECK(from_argv->verbose == from_list->verbose);
    CHECK(from_argv->arg1 == from_list->arg1);
}
