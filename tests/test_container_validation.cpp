#include <print>
#include <starparse/starparse.hpp>

bool is_valid(const std::string &value) {
    return value == "hello" || value == "world";
}

struct Args {
    [[=StarParse::Opt{'v'}, =StarParse::Validator{is_valid}]] std::vector<std::string> vec;
    [[=StarParse::Opt{'a'}, =StarParse::Validator{is_valid}]] std::array<std::string, 2> arr;
};

auto main(int argc, char **argv) -> int {
    const auto args{StarParse::parse<Args>(argc, argv)};
    if (args) {
        std::println("vector: {}, array: {}", args->vec, args->arr);
    } else {
        std::println("{}", args.errors());
    }
}
