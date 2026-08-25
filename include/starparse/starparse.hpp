#include <concepts>
#include <optional>
#include <string_view>
#include <meta>
#include <ranges>
#include <string>
#include <stdexcept>
#include <charconv>
#include <print>

namespace StarParse {
    using namespace std::literals;

    struct Opt {
        char short_name{0};
        const char* help_{};

        consteval Opt(char s, std::string_view h) : short_name(s), help_(std::define_static_string(h)) {}
        constexpr std::string_view help() const { return help_; }
    };

    struct Positional {
        size_t index;
    };

    template <typename M>
    M from_string(std::string_view s) {
        if constexpr (std::constructible_from<M, std::string_view>) {
            return M{s};
        } else if constexpr (std::is_arithmetic_v<M>) {
            M v{};
            auto [pointer, error_code] = std::from_chars(s.data(), s.data() + s.size(), v);
            if (error_code != std::errc{} || pointer != s.data() + s.size()) {
                throw std::invalid_argument(std::format("bad value for type: {}", std::meta::display_string_of(^^M)));
            }
            return v;
        } else {
            static_assert(false, "no conversion for this field type");
        }
    }

    consteval std::optional<Positional> positional_of(std::meta::info m) {
        for (std::meta::info a : std::meta::annotations_of(m)) {
            if (std::meta::dealias(std::meta::remove_cv(std::meta::type_of(a))) == std::meta::dealias(^^Positional)) {
                return std::meta::extract<Positional>(a);
            }
        }
        return std::nullopt;
    }

    consteval std::optional<Opt> opt_of(std::meta::info m) {
        for (std::meta::info a : std::meta::annotations_of(m)) {
            if (std::meta::dealias(std::meta::remove_cv(std::meta::type_of(a))) == std::meta::dealias(^^Opt)) {
                return std::meta::extract<Opt>(a);
            }
        }
        return std::nullopt;
    }

    template <typename T>
    T parse(int argc, char** argv) {
        T out{};
        static constexpr auto members = std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        int next_positional{0};
        bool positional{false};
        for (int i{1}; i < argc; ++i) {
            std::string_view argument{argv[i]};
            const bool is_flag{argument.starts_with("--")};
            if (is_flag) argument.remove_prefix(2);
            bool matched{false};

            template for (constexpr auto m : members) {
                using M = typename [:std::meta::type_of(m):];
                constexpr auto pos = positional_of(m);
                if (is_flag) {
                    constexpr auto opt = opt_of(m);
                    const bool matching_string = argument == std::meta::identifier_of(m) || (argument.size() == 1 && opt.has_value() && argument.at(0) == opt->short_name);
                    if (!matched && matching_string) {
                        matched = true;
                        if constexpr (std::same_as<M, bool>) {
                            out.[:m:] = true;
                        } else {
                            if (i + 1 >= argc) {
                                throw std::invalid_argument(std::format("missing value for option: {}", argument));
                            }
                            out.[:m:] = from_string<M>(argv[++i]);
                        }
                    }
                } else if constexpr (pos.has_value()) {
                    if (!matched && next_positional == pos->index) {
                        matched = true;
                        next_positional++;
                        out.[:m:] = from_string<M>(argv[i]);
                    }
                }
            }
            if (!matched) {
                throw std::invalid_argument(std::format("unrecognized argument: {}", argument));
            }
        }
        return out;
    }
}

