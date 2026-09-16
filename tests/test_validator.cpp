#include <print>
#include <starparse/starparse.hpp>

bool is_valid(const std::string &arg) {
    return arg == "hello";
}

struct Args {
    [[=StarParse::Validator{is_valid}]] std::string model;
};

auto main(int argc, char **argv) -> int {
    constexpr StarParse::Settings settings{.allow_case_insensitivity = true};
    const auto args{StarParse::parse<Args>(argc, argv, settings)};
    std::println("model: {}", args->model);
}
