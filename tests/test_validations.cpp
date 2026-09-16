#include <print>
#include <starparse/starparse.hpp>

// Validator option 1: return bool, no explicit error msg
bool is_valid(const std::string &arg) {
    return arg == "hello";
}

// Validator option 2: return std::expected<void, std::string>, with void on success
std::expected<void, std::string> is_valid_2(const std::string &arg) {
    if (arg == "hello") return {};
    return std::unexpected{"only 'hello' is a valid option"};
}

// Validator option 3: return const char*, with nullptr on success
const char *is_valid_3(const std::string &arg) {
    if (arg == "hello") return nullptr;
    return "only 'hello' is a valid option";
}

struct Args {
    [[=StarParse::Validator{is_valid}]] std::string string1;
    [[=StarParse::Validator{is_valid_2}]] std::string string2;
    [[=StarParse::Validator{is_valid_3}]] std::string string3;
    [[=StarParse::Validator{[](const std::string &arg) -> bool { return arg == "hello"; }}]] std::string string4;
    [[=StarParse::Choices{"a", "b", "c", "d"}]] std::string letter;
    [[=StarParse::Range{1, 3}]] int range;
};

auto main(int argc, char **argv) -> int {
    constexpr StarParse::Settings settings{.allow_case_insensitivity = true};
    const auto args{StarParse::parse_or_exit<Args>(argc, argv, settings)};
    std::println("string1: {}, string2: {}, string3: {}, string4: {}, letter: {}",
                 args->string1, args->string2, args->string3, args->string4, args->letter, args->range);
}
