#include "test_support.hpp"

#include <string>

using namespace StarParse;

namespace {
    struct Args {
        [[=Positional{0}]] std::string positional;
        [[=Opt{"dry run"}]] bool dry_run;
        [[=Opt{'d', "super dry run"}]] bool super_dry_run;
    };
}

TEST_CASE("separator: everything after -- is positional") {
    SUBCASE("-- --foo=bar") {
        const auto args = parse_from<Args>({"--", "--foo=bar"});
        REQUIRE(args);
        CHECK(args->positional == "--foo=bar");
        CHECK_FALSE(args->dry_run);
        CHECK_FALSE(args->super_dry_run);
    }
    SUBCASE("-- -kj4") {
        const auto args = parse_from<Args>({"--", "-kj4"});
        REQUIRE(args);
        CHECK(args->positional == "-kj4");
        CHECK_FALSE(args->dry_run);
        CHECK_FALSE(args->super_dry_run);
    }
}
