#include <print>
#include <starparse/starparse.hpp>
#include <string>

struct Flags {
    [[=StarParse::Positional{0}]] std::string arg1;
    [[=StarParse::Opt{'k', "Keep going"}]] bool keep;
    [[=StarParse::Opt{'v', "Verbose mode"}]] bool verbose;
    [[=StarParse::Opt{'f', "Force"}]] bool force;
    [[=StarParse::Positional{1}]] int arg2;
};

auto main(int argc, char** argv) -> int {
    const auto f{StarParse::immediate_parse<Flags>(argc, argv)};
    std::println("arg1: {}, keep: {}, verbose: {}, force: {}, arg2: {}", f.arg1, f.keep, f.verbose, f.force, f.arg2);
}
