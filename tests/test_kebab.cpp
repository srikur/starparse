#include "test_support.hpp"

using namespace StarParse;

namespace {
    struct Args {
        int dry_run;
    };
}

TEST_CASE("kebab: snake_case fields accept kebab-case spellings") {
    SUBCASE("--dry-run=42") {
        const auto args = parse_from<Args>({"--dry-run=42"});
        REQUIRE(args);
        CHECK(args->dry_run == 42);
    }
    SUBCASE("--dry-run 42") {
        const auto args = parse_from<Args>({"--dry-run", "42"});
        REQUIRE(args);
        CHECK(args->dry_run == 42);
    }
}
