#include <print>
#include <starparse/starparse.hpp>

bool is_valid(const std::string &arg) {
    return arg == "hello";
}

struct Args {
    [[=StarParse::Validator{is_valid}]] std::string model;
    [[=StarParse::Choices{"a", "b", "c", "d"}]] std::string letter;
    [[=StarParse::Range{1, 3}]] int range;
};

auto main(int argc, char **argv) -> int {
    constexpr StarParse::Settings settings{.allow_case_insensitivity = true};
    const auto args{StarParse::parse_or_exit<Args>(argc, argv, settings)};
    std::println("model: {}, letter: {}", args->model, args->letter, args->range);
}
