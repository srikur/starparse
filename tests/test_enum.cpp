#include "test_support.hpp"

#include <string>

using namespace StarParse;

namespace {
    enum class Mode {
        fast [[=Alias{"quick"}]],
        safe,
        dry_run [[=Alias{"dry", "n"}]],
    };

    struct Args {
        [[=Opt{'m', "Archive mode"}]] Mode mode;
        [[=Positional{0}]] std::string input;
    };
}

TEST_CASE("enum: value by enumerator name") {
    const auto args = parse_from<Args>({"--mode=safe", "hello"});
    REQUIRE(args);
    CHECK(args->mode == Mode::safe);
    CHECK(args->input == "hello");
}

TEST_CASE("enum: value by enumerator alias") {
    SUBCASE("--mode quick") {
        const auto args = parse_from<Args>({"--mode", "quick", "hello"});
        REQUIRE(args);
        CHECK(args->mode == Mode::fast);
    }
    SUBCASE("-m dry") {
        const auto args = parse_from<Args>({"-m", "dry", "hello"});
        REQUIRE(args);
        CHECK(args->mode == Mode::dry_run);
    }
    SUBCASE("-m n") {
        const auto args = parse_from<Args>({"-m", "n", "hello"});
        REQUIRE(args);
        CHECK(args->mode == Mode::dry_run);
    }
}

TEST_CASE("enum: unknown value is an invalid-value error") {
    const auto args = parse_from<Args>({"--mode=slow", "hello"});
    REQUIRE_FALSE(args);
    REQUIRE(args.errors().size() == 1uz);
    const auto &error = args.errors()[0];
    CHECK(error.kind == ErrorKind::INVALID_VALUE);
    CHECK(error.input_value == "slow");
    CHECK(error.current_argument.value_or("").ends_with("Mode"));
}
