#include <print>
#include <optional>
#include <starparse/starparse.hpp>

// TODO: static assert: subcommand names can't overlap (might just be covered by not having duplicate field names possible)
struct Add {
    [[=StarParse::Positional{0}]] int value;
};

struct Subtract {
    [[=StarParse::Positional{0}]] int value;
};

struct Operation {
    bool verbose{false};
    [[=StarParse::Subcommand]] std::optional<Add> add;
    [[=StarParse::Subcommand]] std::optional<Subtract> subtract;
};

struct Args {
    [[=StarParse::Required, =StarParse::Positional{0}]] int initial;
    [[=StarParse::Subcommand]] std::optional<Operation> operation;
};

auto main(int argc, char **argv) -> int {
    constexpr StarParse::Settings settings{.allow_case_insensitivity = true};
    const auto args{StarParse::parse_or_exit<Args>(argc, argv, settings)};

    int result = args->initial;
    if (const auto &op = args->operation) {
        if (op->add) result = args->initial + op->add->value;
        if (op->subtract) result = args->initial - op->subtract->value;
    }
    std::println("Result: {}", result);
}
