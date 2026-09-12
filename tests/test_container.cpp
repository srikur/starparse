#include <print>
#include <starparse/starparse.hpp>


struct Args {
    [[=StarParse::Opt{'v', "Vector test"}]] std::vector<std::string> vec;
    [[=StarParse::Opt{'a', "Array test"}, =StarParse::Separator{","}]] std::array<std::string, 4> arr;
};

auto main(int argc, char **argv) -> int {
    const auto args{StarParse::try_parse<Args>(argc, argv)};
    if (args) {
        std::println("vector: {}, array: {}", args->vec, args->arr);
    } else {
        std::println("parse error(s): {}", args.error());
    }
}
