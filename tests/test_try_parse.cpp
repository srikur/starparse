#include <print>
#include <starparse/starparse.hpp>


struct Args {
    [[=StarParse::Opt{'v', "Verbose mode"}]] bool verbose;
    [[=StarParse::Positional{0, "Argument 1 help string"}]] int arg1;
};

auto main(int argc, char **argv) -> int {
    const auto args{StarParse::try_parse<Args>(argc, argv)};
    if (args) {
        std::println("verbose: {}, arg1: {}", args->verbose, args->arg1);
    } else {
        std::println("parse error(s): {}", args.error());
    }
}
