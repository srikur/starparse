#include "test_support.hpp"

#include <array>
#include <string>
#include <vector>

using namespace StarParse;

namespace {
    bool is_valid(const std::string &value) {
        return value == "hello" || value == "world";
    }

    struct Args {
        [[=Opt{'v'}, =Validator{is_valid}]] std::vector<std::string> vec;
        [[=Opt{'a'}, =Validator{is_valid}]] std::array<std::string, 2> arr;
    };
}

TEST_CASE("container validation: each delimited value is validated") {
    const auto args = parse_from<Args>({"--vec=hello,world", "--arr=hello,world"});
    REQUIRE(args);
    const std::vector<std::string> expected_vec{"hello", "world"};
    const std::array<std::string, 2> expected_arr{"hello", "world"};
    CHECK(args->vec == expected_vec);
    CHECK(args->arr == expected_arr);
}

TEST_CASE("container validation: each repeated value is validated") {
    const auto args = parse_from<Args>({"-v", "hello", "-v", "world", "-a", "hello", "-a", "world"});
    REQUIRE(args);
    const std::vector<std::string> expected_vec{"hello", "world"};
    const std::array<std::string, 2> expected_arr{"hello", "world"};
    CHECK(args->vec == expected_vec);
    CHECK(args->arr == expected_arr);
}

TEST_CASE("container validation: a rejected vector element fails the parse") {
    const auto args = parse_from<Args>({"--vec=hello,invalid"});
    REQUIRE_FALSE(args);
    CHECK(args.errors()[0].kind == ErrorKind::VALIDATION_FAILED);
    CHECK(args.error_message() == "Validation failed for input 'invalid': validator returned false");
}

TEST_CASE("container validation: a rejected array element fails the parse") {
    const auto args = parse_from<Args>({"--arr=hello,invalid"});
    REQUIRE_FALSE(args);
    CHECK(args.errors()[0].kind == ErrorKind::VALIDATION_FAILED);
    CHECK(args.error_message() == "Validation failed for input 'invalid': validator returned false");
}
