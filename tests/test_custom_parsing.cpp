#include <chrono>
#include <print>
#include <spanstream>
#include <starparse/starparse.hpp>

// TODO: should it be able to return const char *??
std::expected<std::chrono::seconds, std::string> parse_seconds(const std::string_view &s) {
    using namespace std::chrono;
    if (s.size() < 2) return std::unexpected{"Duration too short"};

    const char unit = s.back();
    if (unit != 's' && unit != 'm')
        return std::unexpected{"Incorrect seconds format. Must end with 's' or 'm'"};

    const auto digits = s.substr(0, s.size() - 1);
    long long n{};
    auto [ptr, ec] = std::from_chars(digits.data(), digits.data() + digits.size(), n);
    if (ec != std::errc{} || ptr != digits.data() + digits.size())
        return std::unexpected{"Unable to parse duration: expected digits before unit"};

    return unit == 's' ? seconds{n} : duration_cast<seconds>(minutes{n});
}

struct Args {
    [[=StarParse::Parser{parse_seconds}]] std::chrono::seconds seconds;
};

auto main(int argc, char **argv) -> int {
    const auto args{StarParse::parse_or_exit<Args>(argc, argv)};
    std::println("{}", args->seconds);
}
