#include <print>
#include <starparse/starparse.hpp>
#include <string>
#include <utility>
#include <optional>

using StarParse::Alias;
using StarParse::Opt;
using StarParse::Positional;

enum class Mode {
    MODE_0, MODE_1, MODE_2, MODE_3 [[=Alias{"m3", "3", "mode3"}]]
};

struct Args {
    [[=Positional{}, =StarParse::Required{}]] std::string arg1;
    [[=Opt{'v', "Verbose mode"}]] bool verbose;
    [[=Opt{'k', "Flag k"}]] bool kay;
    [[=Opt{'f', "Flag f"}]] bool eff;
    [[=Opt{'o'}]] std::optional<std::string> output_file;
    [[=Opt{'m'}]] std::optional<Mode> mode;
    // [[=Opt{'I'}]] std::vector<std::string> include_paths;
};

auto main(int argc, char **argv) -> int {
    const auto args{StarParse::immediate_parse<Args>(argc, argv)};
    std::println("arg1: {}", args.arg1);
    std::println("k: {}, v: {}, f: {}", args.kay, args.verbose, args.eff);
    std::println("mode: {}", args.mode ? std::to_string(std::to_underlying(*args.mode)) : std::string{"nullopt"});
    std::println("output_file: {}", args.output_file.value_or("nullopt"));
    // std::println("include_paths: {}",
    //              args.include_paths.empty() ? "no include paths" : std::to_string(args.include_paths.size()));
}
