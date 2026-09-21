#include "test_support.hpp"

#include <cstdint>
#include <expected>
#include <string>

using namespace StarParse;

namespace {
    bool is_hello(const std::string &arg) {
        return arg == "hello";
    }

    std::expected<void, std::string> is_hello_expected(const std::string &arg) {
        if (arg == "hello") return {};
        return std::unexpected{"only 'hello' is a valid option"};
    }

    const char *is_hello_cstr(const std::string &arg) {
        if (arg == "hello") return nullptr;
        return "only 'hello' is a valid option";
    }

    struct Args {
        [[=Validator{is_hello}]] std::string string1;
        [[=Validator{is_hello_expected}]] std::string string2;
        [[=Validator{is_hello_cstr}]] std::string string3;
        [[=Validator{[](const std::string &arg) -> bool { return arg == "hello"; }}]] std::string string4;
        [[=Choices{"a", "b", "c", "d"}]] std::string letter;
        [[=Range{1, 3}]] int range;
        [[=Range{0, 255}]] uint8_t range2;
    };
}

TEST_CASE("validation: every validator accepts valid input") {
    const auto args = parse_from<Args>({"hello", "hello", "hello", "hello", "a", "2", "255"});
    REQUIRE(args);
    CHECK(args->string1 == "hello");
    CHECK(args->string2 == "hello");
    CHECK(args->string3 == "hello");
    CHECK(args->string4 == "hello");
    CHECK(args->letter == "a");
    CHECK(args->range == 2);
    CHECK(args->range2 == 255);
}

TEST_CASE("validation: bool validator") {
    const auto args = parse_from<Args>({"--string1=nope"});
    REQUIRE_FALSE(args);
    CHECK(args.errors()[0].kind == ErrorKind::VALIDATION_FAILED);
    CHECK(args.error_message() == "Validation failed for input 'nope': validator returned false");
}

TEST_CASE("validation: std::expected validator carries its message") {
    const auto args = parse_from<Args>({"--string2=nope"});
    REQUIRE_FALSE(args);
    CHECK(args.errors()[0].kind == ErrorKind::VALIDATION_FAILED);
    CHECK(args.error_message() == "Validation failed for input 'nope': only 'hello' is a valid option");
}

TEST_CASE("validation: const char* validator carries its message") {
    const auto args = parse_from<Args>({"--string3=nope"});
    REQUIRE_FALSE(args);
    CHECK(args.errors()[0].kind == ErrorKind::VALIDATION_FAILED);
    CHECK(args.error_message() == "Validation failed for input 'nope': only 'hello' is a valid option");
}

TEST_CASE("validation: lambda validator") {
    const auto args = parse_from<Args>({"--string4=nope"});
    REQUIRE_FALSE(args);
    CHECK(args.errors()[0].kind == ErrorKind::VALIDATION_FAILED);
    CHECK(args.error_message() == "Validation failed for input 'nope': validator returned false");
}

TEST_CASE("validation: Choices rejects values outside the set") {
    const auto args = parse_from<Args>({"--letter=z"});
    REQUIRE_FALSE(args);
    CHECK(args.errors()[0].kind == ErrorKind::INVALID_CHOICE);
    CHECK(args.error_message().starts_with("Invalid choice 'z' for argument 'letter'"));
}

TEST_CASE("validation: Range rejects values outside the bounds") {
    SUBCASE("above") {
        const auto args = parse_from<Args>({"--range=4"});
        REQUIRE_FALSE(args);
        CHECK(args.errors()[0].kind == ErrorKind::OUT_OF_RANGE);
    }
    SUBCASE("below") {
        const auto args = parse_from<Args>({"--range=0"});
        REQUIRE_FALSE(args);
        CHECK(args.errors()[0].kind == ErrorKind::OUT_OF_RANGE);
    }
    SUBCASE("value that does not fit the field type at all") {
        const auto args = parse_from<Args>({"--range2=70000"});
        REQUIRE_FALSE(args);
        CHECK(args.errors()[0].kind == ErrorKind::INVALID_VALUE);
    }
}
