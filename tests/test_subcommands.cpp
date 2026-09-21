#include "test_support.hpp"

#include <optional>

using namespace StarParse;

namespace {
    struct Add {
        [[=Positional{0}]] int value;
    };

    struct Subtract {
        [[=Positional{0}]] int value;
    };

    struct Operation {
        bool verbose{false};
        [[=Subcommand, =Alias{"plus"}]] std::optional<Add> add;
        [[=Subcommand, =Alias{"minus"}]] std::optional<Subtract> subtract;
    };

    struct Args {
        [[=Required, =Positional{0}]] int initial;
        [[=Subcommand, =Alias{"op"}]] std::optional<Operation> operation;
    };
}

TEST_CASE("subcommands: nested subcommand with its own positional") {
    const auto check_add = [](const ParsedArgs<Args> &args) {
        REQUIRE(args);
        CHECK(args->initial == 10);
        REQUIRE(args->operation.has_value());
        REQUIRE(args->operation->add.has_value());
        CHECK(args->operation->add->value == 5);
        CHECK_FALSE(args->operation->subtract.has_value());
    };
    SUBCASE("10 operation add 5") {
        check_add(parse_from<Args>({"10", "operation", "add", "5"}));
    }
    SUBCASE("named values at each level") {
        check_add(parse_from<Args>({"--initial=10", "operation", "add", "--value=5"}));
    }
    SUBCASE("subcommand names are case-insensitive") {
        check_add(parse_from<Args>({"10", "OPERATION", "ADD", "5"}));
    }
    SUBCASE("subcommand aliases") {
        check_add(parse_from<Args>({"10", "op", "plus", "5"}));
    }
}

TEST_CASE("subcommands: sibling subcommands are exclusive") {
    const auto check_subtract = [](const ParsedArgs<Args> &args) {
        REQUIRE(args);
        REQUIRE(args->operation.has_value());
        CHECK_FALSE(args->operation->add.has_value());
        REQUIRE(args->operation->subtract.has_value());
        CHECK(args->operation->subtract->value == 5);
    };
    SUBCASE("10 operation subtract 5") {
        check_subtract(parse_from<Args>({"10", "operation", "subtract", "5"}));
    }
    SUBCASE("10 op minus 5") {
        check_subtract(parse_from<Args>({"10", "op", "minus", "5"}));
    }
}

TEST_CASE("subcommands: absent subcommand leaves the optional empty") {
    const auto args = parse_from<Args>({"10"});
    REQUIRE(args);
    CHECK(args->initial == 10);
    CHECK_FALSE(args->operation.has_value());
}

TEST_CASE("subcommands: parent subcommand without a child") {
    const auto args = parse_from<Args>({"10", "operation"});
    REQUIRE(args);
    REQUIRE(args->operation.has_value());
    CHECK_FALSE(args->operation->verbose);
    CHECK_FALSE(args->operation->add.has_value());
    CHECK_FALSE(args->operation->subtract.has_value());
}

TEST_CASE("subcommands: options of the parent subcommand before the child") {
    const auto args = parse_from<Args>({"10", "operation", "--verbose", "add", "5"});
    REQUIRE(args);
    REQUIRE(args->operation.has_value());
    CHECK(args->operation->verbose);
    REQUIRE(args->operation->add.has_value());
    CHECK(args->operation->add->value == 5);
}
