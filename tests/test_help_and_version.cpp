#include <print>
#include <starparse/starparse.hpp>

using namespace StarParse;

struct Args {
    [[=Validator{[](const std::string &arg) -> bool { return arg == "hello"; }}]] std::string string4;
    [[=Choices{"a", "b", "c", "d"}]] std::string letter;
    [[=Range{1, 3}]] int range;
    [[=Range{0, 255}]] uint8_t range2;
};

auto main(int argc, char **argv) -> int {
    const auto opts{StarParse::parse_or_exit<Args>(argc, argv)};
    if (opts.help_requested()) {
        std::println("{}", opts.help());
        return 0;
    }
    if (opts.version_requested()) {
        std::println("{}", opts.version());
        return 0;
    }
}
