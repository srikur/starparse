#include <print>
#include <starparse/starparse.hpp>
#include <string>

struct Args {
    [[=StarParse::Positional{0}]] std::string arg1;
    [[=StarParse::Opt{'v', "Verbose mode"}]] bool verbose; 
    [[=StarParse::Positional{1}]] int arg2;
};

struct Args2 {
    std::string arg1;
    bool verbose;
    int arg2;
};

auto main(int argc, char** argv) -> int {
    const auto args{StarParse::immediate_parse<Args>(argc, argv)};
    std::println("arg1: {}, verbose: {}, arg2: {}", args.arg1, args.verbose, args.arg2);
}
