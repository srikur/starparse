#include <print>
#include <starparse/starparse.hpp>
#include <string>

// struct Args {
//     [[=StarParse::Positional{0}]] std::string arg1;
//     [[=StarParse::Opt{'v', "Verbose mode"}]] bool verbose; 
//     [[=StarParse::Positional{1}]] int arg2;
// };

struct Args2 {
    std::string arg1;
    [[=StarParse::Opt{'v', "Verbose mode"}]] bool verbose;
    [[=StarParse::Opt{'k', "Flag k"}]] bool kay;
    [[=StarParse::Opt{'f', "Flag f"}]] bool eff;
    int arg2;
};

auto main(int argc, char** argv) -> int {
    // const auto args{StarParse::immediate_parse<Args>(argc, argv)};
    // std::println("Args  arg1: {}, verbose: {}, arg2: {}", args.arg1, args.verbose, args.arg2);

    const auto args2{StarParse::immediate_parse<Args2>(argc, argv)};
    std::println("Args2 k: {}, v: {}, f: {}", args2.kay, args2.verbose, args2.eff);
}
