#include "test_support.hpp"

#include <array>
#include <string>
#include <vector>

using namespace StarParse;

namespace {
    struct Args {
        [[=Opt{'v', "Vector test"}]] std::vector<std::string> vec;
        [[=Opt{'a', "Array test"}, =Separator{","}]] std::array<std::string, 4> arr;
    };
}

TEST_CASE("container: delimited values fill vectors and arrays") {
    const auto args = parse_from<Args>({"--vec=1,2,3,4", "--arr=1,2,3,4"});
    REQUIRE(args);
    const std::vector<std::string> expected_vec{"1", "2", "3", "4"};
    const std::array<std::string, 4> expected_arr{"1", "2", "3", "4"};
    CHECK(args->vec == expected_vec);
    CHECK(args->arr == expected_arr);
}

TEST_CASE("container: Settings::value_separator sets the default delimiter") {
    const auto args = parse_from<Args>({"--vec=1;2;3;4", "--arr=1,2,3,4"}, Settings{.value_separator = ";"});
    REQUIRE(args);
    const std::vector<std::string> expected_vec{"1", "2", "3", "4"};
    const std::array<std::string, 4> expected_arr{"1", "2", "3", "4"};
    CHECK(args->vec == expected_vec);
    CHECK(args->arr == expected_arr);
}
