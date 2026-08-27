#include <print>
#include <starparse/starparse.hpp>
#include <string>
#include <utility>

enum class Mode {
    fast [[=StarParse::Alias{"quick"}]],
    safe,
    dry_run [[=StarParse::Alias{"dry", "n"}]],
};

struct Args {
    [[=StarParse::Opt{'m', "Archive mode"}]] Mode mode;
    [[=StarParse::Positional{0}]] std::string input;
};

auto main(int argc, char** argv) -> int {
    try {
        const auto args{StarParse::immediate_parse<Args>(argc, argv)};
        std::println("mode: {}, input: {}", std::to_underlying(args.mode), args.input);
    } catch (const std::exception& e) {
        std::println("error: {}", e.what());
    }
}
