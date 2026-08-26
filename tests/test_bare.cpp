#include <print>
#include <starparse/starparse.hpp>
#include <string>

struct Bare {
    std::string arg1;
    bool verbose;
    int arg2;
};

auto main(int argc, char** argv) -> int {
    const auto args{StarParse::immediate_parse<Bare>(argc, argv)};
    std::println("Bare arg1: {}, verbose: {}, arg2: {}", args.arg1, args.verbose, args.arg2);
}
