#include "test_support.hpp"

using namespace StarParse;

namespace {
    struct Args {
        [[=Validator{[](const std::string &arg) -> bool { return arg == "hello"; }}]] std::string string4;
        [[=Choices{"a", "b", "c", "d"}]] std::string letter;
        [[=Range{1, 3}]] int range;
        [[=Range{0, 255}]] uint8_t range2;
    };

    struct Args2 {
        [[=Validator{[](const std::string &arg) -> bool { return arg == "hello"; }}]] std::string string4;
        [[=Choices{"a", "h", "c", "d"}]] std::string letter;
        [[=Range{1, 3}]] int range;
        [[=Range{0, 255}]] uint8_t range2;
    };

    constexpr Settings settings{.allow_case_insensitivity = true};
}

TEST_CASE("help and version: check short form works") {
    const auto args = parse_from<Args>({"-H"}, settings);
    REQUIRE(args);
    CHECK(args.help_requested() == true);
    CHECK(args.version_requested() == false);
}

TEST_CASE("help and version: check short form disabled when used by an argument") {
    const auto args = parse_from<Args2>({"-h"}, settings);
    REQUIRE_FALSE(args);
    CHECK(args.errors().size() == 1uz);
    CHECK(args.errors()[0].kind == ErrorKind::UNKNOWN_OPTION);
}
