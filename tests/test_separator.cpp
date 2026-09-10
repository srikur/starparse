#include <print>
#include <starparse/starparse.hpp>

struct Args {
    [[=StarParse::Positional{0}]] std::string positional;
};

auto main(int argc, char **argv) -> int {
    const auto args{StarParse::try_parse<Args>(argc, argv)};
    if (args) {
        std::println("positional: {}", args->positional);
    } else {
        std::println("parse error(s): {}", args.error());
    }
}
