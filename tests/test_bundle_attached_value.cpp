#include <print>
#include <starparse/starparse.hpp>
#include <string>

struct Flags {
    [[=StarParse::Opt{'k', "Keep going"}, =StarParse::Alias{"q"}]] bool keep;
    [[=StarParse::Opt{'j', "Num jobs"}, =StarParse::Alias{"n"}]] int jobs;
};

auto main(int argc, char **argv) -> int {
    try {
        const auto f{StarParse::parse_or_throw<Flags>(argc, argv)};
        std::println("keep: {}, jobs: {}", f->keep, f->jobs);
    } catch (const std::exception &e) {
        std::println("error: {}", e.what());
    }
}
