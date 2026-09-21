#include "test_support.hpp"

#include <array>
#include <charconv>
#include <chrono>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

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

    struct ContainerArgs {
        [[=Parser{parse_duration}]] std::vector<std::chrono::seconds> vec_durations;
        [[=Parser{parse_duration}]] std::array<std::chrono::seconds, 3> arr_durations;
    };

    bool is_positive(const std::chrono::seconds &value) {
        return value > std::chrono::seconds{0};
    }

    struct ValidatedContainerArgs {
        [[=Parser{parse_duration}, =Validator{is_positive}]] std::vector<std::chrono::seconds> vec;
        [[=Parser{parse_duration}, =Validator{is_positive}]] std::array<std::chrono::seconds, 2> arr;
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

TEST_CASE("custom parsing: vectors and arrays have each value parsed") {
    SUBCASE("vector") {
        const auto args = parse_from<ContainerArgs>({"--vec_durations=90s,2m,45s"});
        REQUIRE(args);
        const std::vector expected{
            std::chrono::seconds{90}, std::chrono::seconds{120}, std::chrono::seconds{45}
        };
        CHECK(args->vec_durations == expected);
    }
    SUBCASE("array") {
        const auto args = parse_from<ContainerArgs>({"--arr_durations=90s,2m,45s"});
        REQUIRE(args);
        constexpr std::array expected{std::chrono::seconds{90}, std::chrono::seconds{120}, std::chrono::seconds{45}};
        CHECK(args->arr_durations == expected);
    }
}

TEST_CASE("custom parsing: repeated options append parsed elements") {
    const auto args = parse_from<ContainerArgs>({
        "--vec_durations=90s", "--vec_durations=2m,45s",
        "--arr_durations=90s", "--arr_durations=2m,45s"
    });
    REQUIRE(args);
    const std::vector expected_vec{
        std::chrono::seconds{90}, std::chrono::seconds{120}, std::chrono::seconds{45}
    };
    constexpr std::array expected_arr{std::chrono::seconds{90}, std::chrono::seconds{120}, std::chrono::seconds{45}};
    CHECK(args->vec_durations == expected_vec);
    CHECK(args->arr_durations == expected_arr);
}

TEST_CASE("custom parsing: container errors identify the failing element and option") {
    std::string_view input;
    std::string_view name;
    SUBCASE("vector") {
        input = "--vec_durations=90s,x5s,45s";
        name = "vec_durations";
    }
    SUBCASE("array") {
        input = "--arr_durations=90s,x5s,45s";
        name = "arr_durations";
    }
    const auto args = parse_from<ContainerArgs>({"--vec_durations=2m", input});
    REQUIRE_FALSE(args);
    REQUIRE(args.errors().size() == 1);
    const auto &error = args.errors()[0];
    CHECK(error.kind == ErrorKind::CUSTOM_PARSING_FAILED);
    CHECK(error.input_value == "x5s");
    CHECK(error.current_argument == name);
    CHECK(error.argv_index == 2);
    CHECK(args.error_message() ==
        "Annotated parser failed for input 'x5s': Unable to parse duration: expected digits before unit");
}

TEST_CASE("custom parsing: validators check each parsed container element") {
    SUBCASE("valid elements") {
        const auto args = parse_from<ValidatedContainerArgs>({"--vec=90s,2m", "--arr=90s,2m"});
        REQUIRE(args);
        const std::vector expected_vec{std::chrono::seconds{90}, std::chrono::seconds{120}};
        constexpr std::array expected_arr{std::chrono::seconds{90}, std::chrono::seconds{120}};
        CHECK(args->vec == expected_vec);
        CHECK(args->arr == expected_arr);
    }
    SUBCASE("rejected vector element") {
        const auto args = parse_from<ValidatedContainerArgs>({"--vec=90s,0s"});
        REQUIRE_FALSE(args);
        REQUIRE(args.errors().size() == 1);
        CHECK(args.errors()[0].kind == ErrorKind::VALIDATION_FAILED);
        CHECK(args.errors()[0].input_value == "0s");
    }
    SUBCASE("rejected array element") {
        const auto args = parse_from<ValidatedContainerArgs>({"--arr=90s,0s"});
        REQUIRE_FALSE(args);
        REQUIRE(args.errors().size() == 1);
        CHECK(args.errors()[0].kind == ErrorKind::VALIDATION_FAILED);
        CHECK(args.errors()[0].input_value == "0s");
    }
}
