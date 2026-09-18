#include <print>
#include <optional>
#include <starparse/starparse.hpp>

struct Add {
    [[=StarParse::Positional{0}]] int value;
};

struct Subtract {
    [[=StarParse::Positional{0}]] int value;
};

struct Operation {
    bool verbose{false};
    [[=StarParse::Subcommand, =StarParse::Alias{"plus"}]] std::optional<Add> add;
    [[=StarParse::Subcommand, =StarParse::Alias{"minus"}]] std::optional<Subtract> subtract;
};

struct Args {
    [[=StarParse::Required, =StarParse::Positional{0}]] int initial;
    [[=StarParse::Subcommand, =StarParse::Alias{"op"}]] std::optional<Operation> operation;
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
