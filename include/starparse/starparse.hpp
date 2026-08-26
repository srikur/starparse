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

    enum class DashType { SINGLE, DOUBLE, BOTH, NONE };

    struct Settings {
        DashType dash_type{DashType::BOTH};
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

    template<typename T>
    bool option_takes_value(std::string_view name, bool is_short) {
        static constexpr auto members = std::define_static_array(
                std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        bool takes{false};
        template for (constexpr auto m : members) {
            using M = typename [:std::meta::type_of(m):];
            constexpr auto opt = opt_of(m);
            const bool match = is_short ? (name.size() == 1 && opt.has_value() && name[0] == opt->short_name) : name == std::meta::identifier_of(m);
            if (match) takes = !std::same_as<M, bool>;
        }
        return takes;
    }

    struct ArgAttributes {
        bool dashed{};
        bool double_dashed{};
        bool is_help{};
        bool is_positional{};
        bool is_separator{};
        std::string_view name{};
        std::string_view value{};

        explicit ArgAttributes(std::string_view argument) {
            if (!argument.starts_with("-")) {
                is_positional = true;
                name = argument;
                return;
            }

            if (argument.starts_with("-") && argument.size() > 1 && argument.at(1) != '-') {
                dashed = true;
                argument.remove_prefix(1);
                name = argument;
            } else if (argument.starts_with("--")) {
                if (argument.size() == 2) {
                    is_separator = true;
                }
                else {
                    double_dashed = true;
                    argument.remove_prefix(2);
                    name = argument;
                }
                if (name == "help") {
                    is_help = true;
                }
            }
        }
    };

    struct ArgContext {
        bool separator_seen{};
    };

    enum class ErrorKind {
        UNKNOWN_OPTION, MISSING_VALUE, INVALID_VALUE, UNEXPECTED_POSITIONAL, MISSING_REQUIRED, DUPLICATE_OPTION
    };

    struct ParseError {
        ErrorKind kind;
        std::string_view token;
        std::string_view option;
        int argv_index;
    };

    template<typename T>
    class ParsedArgs {
    public:
        ParsedArgs(T out, bool show_help, std::vector<ParseError>& errors) : out_(std::move(out)), show_help_(show_help), errors_(std::move(errors)) {}
        explicit operator bool() const {
            return out_ == T{};
        }
        T& operator*() {
            return out_;
        }
        const T& operator*() const {
            return out_;
        }
        T&& value() && {
            return out_;
        }
        std::span<const ParseError> errors() const {
            return errors_;
        }
        bool help_requested() {
            return show_help_;
        }
    private:
        T out_{};
        bool show_help_{};
        std::vector<ParseError> errors_;
    };

    template <typename T>
    ParsedArgs<T> parse(int argc, char** argv, Settings settings = {}) {
        T out{};
        bool help_requested{};
        std::vector<ParseError> errors{};
        static constexpr auto members = std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        size_t next_positional{0};
        ArgContext ctx{};


        std::vector<ArgAttributes> attr_array;
        attr_array.reserve(argc - 1);
        bool separator_seen{false};
        for (int i{1}; i < argc; ++i) {
            ArgAttributes a{argv[i]};
            if (a.is_separator) {
                separator_seen = true;
                continue;
            }
            if (separator_seen) {
                a.is_positional = true;
            }
            if ((a.dashed || a.double_dashed) && a.value.empty() && option_takes_value<T>(a.name, a.dashed) && i + 1 < argc) {
                a.value = argv[++i];
            }
            attr_array.push_back(a);
        }

        for (const auto& attrs : attr_array) {
            bool matched{false};
            template for (constexpr auto m : members) {
                using M = typename [:std::meta::type_of(m):];
                constexpr auto pos = positional_of(m);
                if (attrs.is_positional || ctx.separator_seen) {
                    if constexpr (pos.has_value()) {
                        if (!matched && next_positional == pos->index) {
                            matched = true;
                            next_positional++;
                            out.[:m:] = from_string<M>(attrs.name);
                        }
                    }
                } else if (attrs.is_separator) {
                    ctx.separator_seen = true;
                } else if (attrs.dashed) {
                    constexpr auto opt = opt_of(m);
                    const bool matching_string = attrs.name == std::meta::identifier_of(m) || (attrs.name.size() == 1 && opt.has_value() && attrs.name[0] == opt->short_name);
                    if (!matched && matching_string) {
                        matched = true;
                        if constexpr (std::same_as<M, bool>) {
                            out.[:m:] = true;
                        } else {
                            if (attrs.value.empty()) {
                                throw std::invalid_argument(std::format("missing value for option: {}", attrs.name));
                            }
                            out.[:m:] = from_string<M>(attrs.value);
                        }
                    }
                }
            }
            if (!matched) {
                throw std::invalid_argument(std::format("unrecognized argument: {}", attrs.name));
            }
        }
        return ParsedArgs<T>{out, help_requested, errors};
    }
}

