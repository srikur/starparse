#include <print>
#include <starparse/starparse.hpp>
#include <string>
#include <utility>

enum class Mode {
    MODE_0, MODE_1, MODE_2, MODE_3 [[=StarParse::Alias{"m3", "3", "mode3"}]]
};

struct Args {
    std::string arg1;
    [[=StarParse::Opt{'v', "Verbose mode"}]] bool verbose;
    [[=StarParse::Opt{'k', "Flag k"}]] bool kay;
    [[=StarParse::Opt{'f', "Flag f"}]] bool eff;
    int arg2;
    [[=StarParse::Opt{'v'}]] Mode mode;
};

auto main(int argc, char **argv) -> int {
    const auto args{StarParse::immediate_parse<Args>(argc, argv)};
    std::println("k: {}, v: {}, f: {}", args.kay, args.verbose, args.eff);
    std::println("mode: {}", std::to_underlying(args.mode));
}
