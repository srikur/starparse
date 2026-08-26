#include <print>
#include <starparse/starparse.hpp>
#include <string>

struct Flags {
    [[=StarParse::Positional{0}]] std::string arg1;
    [[=StarParse::Opt{'k', "Keep going"}]] bool keep;
    [[=StarParse::Opt{'v', "Verbose mode"}]] bool verbose;
    [[=StarParse::Positional{1}]] int arg2;
};

auto main(int argc, char** argv) -> int {
    Flags defaults{.arg1 = "default1", .keep = true, .verbose = false, .arg2 = 99};
    const auto f{StarParse::immediate_parse(argc, argv, defaults)};
    std::println("arg1: {}, keep: {}, verbose: {}, arg2: {}", f.arg1, f.keep, f.verbose, f.arg2);
}
