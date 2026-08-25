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
        consteval std::string_view help() const { return help_; }
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
                throw std::invalid_argument("bad value");
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
                    if (!matched && argument == std::meta::identifier_of(m)) {
                        matched = true;
                        if constexpr (std::same_as<M, bool>) {
                            out.[:m:] = true;
                        } else {
                            if (i + 1 >= argc) {
                                throw std::invalid_argument("missing value for option");
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
                throw std::invalid_argument("unrecognized argument");
            }
        }
        return out;
    }
}

