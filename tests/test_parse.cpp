#include <print>
#include <starparse/starparse.hpp>
#include <string>
#include <utility>
#include <optional>

enum class Mode {
    MODE_0, MODE_1, MODE_2, MODE_3 [[=StarParse::Alias{"m3", "3", "mode3"}]]
};

struct [[=StarParse::Program{"starparse", "CMD line arg paraer", "0.0.1"}]] Args {
    [[=StarParse::Positional{0}]] std::string arg1;
    [[=StarParse::Opt{'v', "Verbose mode"}]] bool verbose;
    [[=StarParse::Opt{'k', "Flag k"}]] bool kay;
    [[=StarParse::Opt{'f', "Flag f"}]] bool eff;
    [[=StarParse::Opt{'o'}]] std::optional<std::string> output_file;
    [[=StarParse::Opt{'m'}]] std::optional<Mode> mode;
    [[=StarParse::Opt{'I'}]] std::vector<std::string> include_paths;
};

auto main(int argc, char **argv) -> int {
    const auto args{StarParse::force_parse<Args>(argc, argv)};
    std::println("arg1: {}", args.arg1);
    std::println("k: {}, v: {}, f: {}", args.kay, args.verbose, args.eff);
    std::println("mode: {}", args.mode ? std::to_string(std::to_underlying(*args.mode)) : std::string{"nullopt"});
    std::println("output_file: {}", args.output_file.value_or("nullopt"));
    std::println("include_paths: {}", args.include_paths);
}
