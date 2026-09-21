#include "test_support.hpp"

#include <charconv>
#include <chrono>
#include <expected>
#include <string>
#include <string_view>

using namespace StarParse;

namespace {
    std::expected<std::chrono::seconds, std::string> parse_duration(const std::string_view &text) {
        using namespace std::chrono;
        if (text.size() < 2) return std::unexpected{"Duration too short"};

        const char unit = text.back();
        if (unit != 's' && unit != 'm')
            return std::unexpected{"Incorrect seconds format. Must end with 's' or 'm'"};

        const auto digits = text.substr(0, text.size() - 1);
        long long n{};
        const auto [ptr, ec] = std::from_chars(digits.data(), digits.data() + digits.size(), n);
        if (ec != std::errc{} || ptr != digits.data() + digits.size())
            return std::unexpected{"Unable to parse duration: expected digits before unit"};

        return unit == 's' ? seconds{n} : duration_cast<seconds>(minutes{n});
    }

    struct Args {
        [[=Parser{parse_duration}]] std::chrono::seconds duration;
    };
}

TEST_CASE("custom parsing: the annotated parser converts the value") {
    SUBCASE("seconds") {
        const auto args = parse_from<Args>({"90s"});
        REQUIRE(args);
        CHECK(args->duration == std::chrono::seconds{90});
    }
    SUBCASE("minutes") {
        const auto args = parse_from<Args>({"2m"});
        REQUIRE(args);
        CHECK(args->duration == std::chrono::seconds{120});
    }
}

TEST_CASE("custom parsing: the parser's error message is reported") {
    SUBCASE("bad unit") {
        const auto args = parse_from<Args>({"abc"});
        REQUIRE_FALSE(args);
        CHECK(args.errors()[0].kind == ErrorKind::CUSTOM_PARSING_FAILED);
        CHECK(args.error_message() ==
            "Annotated parser failed for input 'abc': Incorrect seconds format. Must end with 's' or 'm'");
    }
    SUBCASE("too short") {
        const auto args = parse_from<Args>({"s"});
        REQUIRE_FALSE(args);
        CHECK(args.error_message() == "Annotated parser failed for input 's': Duration too short");
    }
    SUBCASE("bad digits") {
        const auto args = parse_from<Args>({"x5s"});
        REQUIRE_FALSE(args);
        CHECK(args.error_message() ==
            "Annotated parser failed for input 'x5s': Unable to parse duration: expected digits before unit");
    }
}
