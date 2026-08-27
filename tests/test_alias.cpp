#include <print>
#include <starparse/starparse.hpp>
#include <string>

struct Args {
    [[=StarParse::Positional{0}, =StarParse::Alias{"source"}]] std::string input;
    [[=StarParse::Opt{'v', "Verbose mode"}, =StarParse::Alias{"talkative", "q"}]] bool verbose;
    [[=StarParse::Opt{'k', "Keep temporary files"}]] bool keep;
    [[=StarParse::Positional{1}, =StarParse::Alias{"count"}]] int arg2;
};

auto main(int argc, char** argv) -> int {
    const auto args{StarParse::immediate_parse<Args>(argc, argv)};
    std::println("input: {}, verbose: {}, keep: {}, arg2: {}", args.input, args.verbose, args.keep, args.arg2);
}
