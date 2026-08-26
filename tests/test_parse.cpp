#include <print>
#include <starparse/starparse.hpp>
#include <string>

struct Args {
    [[=StarParse::Positional{0}]] std::string arg1;
    [[=StarParse::Opt{'v', "Verbose mode"}]] bool verbose; 
    [[=StarParse::Positional{1}]] int arg2;
};

auto main(int argc, char** argv) -> int {
    const auto result{StarParse::parse<Args>(argc, argv)};
    // if (result.help_requested()) {
    //     std::print("{}", StarParse::help_text<Args>());
    //     return 0;
    // }
    // if (!result) {
    //     std::print(result.errors());
    // }
    const auto& args = *result;
    std::println("arg1: {}, verbose: {}, arg2: {}", args.arg1, args.verbose, args.arg2);
}
