#include <print>
#include <starparse/starparse.hpp>
#include <string>

//  basic <input> [output] [-v] [-f] [-j N | --jobs=N | --jobs N]
struct Options {
    [[=StarParse::Positional{0}]] std::string input;
    [[=StarParse::Positional{1}]] std::string output;
    [[=StarParse::Opt{'v', "Enable verbose logging"}]] bool verbose;
    [[=StarParse::Opt{'f', "Overwrite the output if it exists"}]] bool force;
    [[=StarParse::Opt{'j', "Number of worker threads"}]] int jobs;
};

auto main(int argc, char** argv) -> int {
    // Fields not mentioned on the command line keep these values
    Options defaults{.output = "archive.out", .jobs = 1};
    const auto opts{StarParse::force_parse(argc, argv, defaults)};

    std::println("input:   {}", opts.input);
    std::println("output:  {}", opts.output);
    std::println("verbose: {}", opts.verbose);
    std::println("force:   {}", opts.force);
    std::println("jobs:    {}", opts.jobs);
}
