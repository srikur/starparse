#include <print>
#include <starparse/starparse.hpp>

struct Args {
    [[=StarParse::Positional{0}]] std::string positional;
    [[=StarParse::Opt{"dry run"}]] bool dry_run;
    [[=StarParse::Opt{'d', "super dry run"}]] bool super_dry_run;
};

auto main(int argc, char **argv) -> int {
    const auto args{StarParse::try_parse<Args>(argc, argv)};
    if (args) {
        std::println("positional: {}, dry run: {}, super dry: {}", args->positional, args->dry_run,
                     args->super_dry_run);
    } else {
        std::println("parse error(s): {}", args.error());
    }
}
