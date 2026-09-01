#pragma once

#include <algorithm>
#include <concepts>
#include <optional>
#include <string_view>
#include <meta>
#include <ranges>
#include <string>
#include <stdexcept>
#include <charconv>
#include <print>
#include <type_traits>
#include <array>
#include <vector>

namespace StarParse {
    using namespace std::literals;

    struct Opt final {
        char short_name{0};
        const char *help_{};

        explicit consteval Opt(const char s, std::string_view h) : short_name(s), help_(std::define_static_string(h)) {}

        explicit consteval Opt(std::string_view h) : help_(std::define_static_string(h)) {}

        explicit consteval Opt(const char s) : short_name(s) {}

        [[nodiscard]] constexpr std::string_view help() const { return help_; }
    };

    struct Positional final {
        size_t index;
    };

    struct Required final {};

    struct Alias final {
        const char *const*names_{};
        size_t count_{};

        template<std::convertible_to<std::string_view>... Ts>
            requires (sizeof...(Ts) > 0)
        consteval Alias(Ts... ns)
            : names_(std::define_static_array(
                  std::array{std::define_static_string(std::string_view{ns})...}).data()),
              count_(sizeof...(ns)) {}
    };

    struct Program final {
        explicit consteval
        Program(std::string_view name, std::string_view description, std::string_view version) : name_(
                std::define_static_string(name)), description_(std::define_static_string(description)),
            version_(std::define_static_string(version)) {}

        const char *name_{};
        const char *description_{};
        const char *version_{};
    };

    consteval std::vector<const char *> alias_name_list(std::meta::info m) {
        std::vector<const char *> names{};
        for (const std::meta::info a : std::meta::annotations_of(m)) {
            if (std::meta::dealias(std::meta::remove_cv(std::meta::type_of(a))) != std::meta::dealias(^^Alias)) {
                continue;
            }
            const auto alias = std::meta::extract<Alias>(a);
            for (size_t i{0}; i < alias.count_; i++) {
                names.push_back(alias.names_[i]);
            }
        }
        return names;
    }

    template<std::meta::info M>
    bool matches_alias(const std::string_view name) {
        static constexpr auto aliases = std::define_static_array(alias_name_list(M));
        for (const char *alias : aliases) {
            if (name == std::string_view{alias}) return true;
        }
        return false;
    }

    consteval bool is_optional(std::meta::info r) {
        r = std::meta::dealias(r);
        return std::meta::has_template_arguments(r) && std::meta::template_of(r) == ^^std::optional;
    }

    enum class DashType { SINGLE, DOUBLE, BOTH, NONE };

    struct Settings {
        DashType dash_type{DashType::BOTH};
        bool allow_kebab_casing{true};
        bool allow_upper_casing{true};
        bool allow_aliases{true};
    };

    consteval std::meta::info value_type_of(const std::meta::info r) {
        return std::meta::template_arguments_of(std::meta::dealias(r))[0];
    }

    consteval bool is_flag_type(std::meta::info r) {
        r = std::meta::dealias(std::meta::remove_cv(r));
        if (is_optional(r)) {
            r = std::meta::dealias(value_type_of(r));
        }
        return r == std::meta::dealias(^^bool);
    }

    template<typename M>
    M from_string(std::string_view s) {
        if constexpr (is_optional(^^M)) {
            using T = [:value_type_of(^^M):];
            return M{from_string<T>(s)};
        } else if constexpr (std::same_as<M, bool>) {
            static_assert(false, "bool fields are flags set by presence; they cannot be parsed from a value");
        } else if constexpr (std::constructible_from<M, std::string_view>) {
            return M{s};
        } else if constexpr (std::is_arithmetic_v<M>) {
            M v{};
            auto [pointer, error_code] = std::from_chars(s.data(), s.data() + s.size(), v);
            if (error_code != std::errc{} || pointer != s.data() + s.size()) {
                throw std::invalid_argument(std::format("bad value for type: {}", std::meta::display_string_of(^^M)));
            }
            return v;
        } else if constexpr (std::is_enum_v<M>) {
            std::optional<M> parsed;
            template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^M))) {
                if (!parsed.has_value() && (s == std::meta::identifier_of(e) || matches_alias<e>(s))) {
                    parsed = [:e:];
                }
            }
            if (!parsed)
                throw std::invalid_argument(
                    std::format("bad value for type: {}", std::meta::display_string_of(^^M)));
            return *parsed;
        } else {
            static_assert(false, "no conversion for this field type");
        }
    }

    consteval std::optional<Positional> positional_of(std::meta::info m) {
        for (const std::meta::info a : std::meta::annotations_of(m)) {
            if (std::meta::dealias(std::meta::remove_cv(std::meta::type_of(a))) == std::meta::dealias(^^Positional)) {
                return std::meta::extract<Positional>(a);
            }
        }
        return std::nullopt;
    }

    consteval std::optional<Opt> opt_of(std::meta::info m) {
        for (const std::meta::info a : std::meta::annotations_of(m)) {
            if (std::meta::dealias(std::meta::remove_cv(std::meta::type_of(a))) == std::meta::dealias(^^Opt)) {
                return std::meta::extract<Opt>(a);
            }
        }
        return std::nullopt;
    }

    template<typename T>
    consteval bool is_bare() {
        for (const std::meta::info m :
             std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current())) {
            if (opt_of(m).has_value() || positional_of(m).has_value()) {
                return false;
            }
        }
        return true;
    }

    template<typename T>
    consteval bool is_named_option(std::meta::info m) {
        return opt_of(m).has_value() || positional_of(m).has_value() || is_bare<T>();
    }

    template<typename T>
    consteval std::optional<size_t> positional_index_of(std::meta::info m) {
        if (auto pos = positional_of(m)) {
            return pos->index;
        }
        if (!is_bare<T>() || is_flag_type(std::meta::type_of(m))) {
            return std::nullopt;
        }
        size_t index{0};
        for (std::meta::info member : std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current())) {
            if (is_flag_type(std::meta::type_of(member))) {
                continue;
            }
            if (member == m) {
                return index;
            }
            ++index;
        }
        return std::nullopt;
    }

    template<typename T>
    consteval size_t member_index_of(const std::meta::info m) {
        size_t index{0};
        for (const std::meta::info member :
             std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current())) {
            if (member == m) {
                return index;
            }
            ++index;
        }
        throw std::invalid_argument("member not found");
    }

    template<typename T>
    bool option_takes_value(std::string_view name, bool is_short) {
        static constexpr auto members = std::define_static_array(
            std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        bool takes{false};
        template for (constexpr auto m : members) {
            using M = [:std::meta::type_of(m):];
            constexpr auto opt = opt_of(m);
            constexpr bool named = is_named_option<T>(m);
            const bool match = (named && (name == std::meta::identifier_of(m) || matches_alias<m>(name))) || (
                                   is_short && name.size() == 1 && opt.has_value() && name[0] == opt->short_name);
            if (match) takes = !is_flag_type(^^M);
        }
        return takes;
    }

    consteval bool is_required(const std::meta::info m) {
        for (const std::meta::info a : std::meta::annotations_of(m)) {
            if (std::meta::dealias(std::meta::remove_cv(std::meta::type_of(a))) == std::meta::dealias(^^Required)) {
                return true;
            }
        }
        return false;
    }

    template<typename T>
    consteval bool no_required_optionals() {
        for (const std::meta::info m :
             std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current())) {
            if (is_required(m) && is_optional(std::meta::remove_cv(std::meta::type_of(m)))) {
                return false;
            }
        }
        return true;
    }

    struct ArgAttributes {
        bool dashed{};
        bool double_dashed{};
        bool is_help{};
        bool is_positional{};
        bool is_separator{};
        bool has_value{};
        std::string_view name{};
        std::string_view value{};

        explicit ArgAttributes(std::string_view argument) {
            if (!argument.starts_with('-')) {
                is_positional = true;
                name = argument;
                return;
            }

            if (argument.starts_with('-') && argument.size() > 1 && argument.at(1) != '-') {
                dashed = true;
                argument.remove_prefix(1);
                name = argument;
            } else if (argument.starts_with("--")) {
                if (argument.size() == 2) {
                    is_separator = true;
                } else {
                    double_dashed = true;
                    argument.remove_prefix(2);
                    name = argument;
                }
            }

            if (const auto equals = name.find('='); equals != std::string_view::npos) {
                value = name.substr(equals + 1);
                has_value = true;
                name = name.substr(0, equals);
            }
            if (double_dashed && name == "help") {
                is_help = true;
            }
        }
    };

    template<typename T>
    bool matches_full_name(std::string_view name) {
        static constexpr auto members = std::define_static_array(
            std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        bool found{false};
        template for (constexpr auto m : members) {
            constexpr bool named = is_named_option<T>(m);
            if (named && (name == std::meta::identifier_of(m) || matches_alias<m>(name))) {
                found = true;
            }
        }
        return found;
    }

    template<typename T>
    bool is_flag_bundle(std::string_view name) {
        static constexpr auto members = std::define_static_array(
            std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        if (name.size() < 2) {
            return false;
        }
        for (const char c : name) {
            bool is_flag{false};
            template for (constexpr auto m : members) {
                using M = [:std::meta::type_of(m):];
                constexpr auto opt = opt_of(m);
                if constexpr (is_flag_type(^^M)) {
                    if constexpr (opt.has_value()) {
                        if (c == opt->short_name) {
                            is_flag = true;
                        }
                    }
                    if constexpr (is_named_option<T>(m)) {
                        if (matches_alias<m>(std::string_view{&c, 1})) {
                            is_flag = true;
                        }
                    }
                }
            }
            if (!is_flag) {
                return false;
            }
        }
        return true;
    }

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
        ParsedArgs(T out, const bool show_help, std::vector<ParseError> &errors) : out_(std::move(out)),
            show_help_(show_help),
            errors_(std::move(errors)) {}

        explicit operator bool() const {
            return out_ == T{};
        }

        T &operator*() {
            return out_;
        }

        const T &operator*() const {
            return out_;
        }

        T &&value() && {
            return std::move(out_);
        }

        [[nodiscard]] std::span<const ParseError> errors() const {
            return errors_;
        }

        [[nodiscard]] bool help_requested() const {
            return show_help_;
        }

    private:
        T out_{};
        bool show_help_{};
        std::vector<ParseError> errors_;
    };

    template<typename T>
    std::vector<ArgAttributes> get_arg_attrs(const int argc, char **argv, const Settings &settings) {
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
                // post-separator, all args are positional
                a.is_positional = true;
            } else if (a.dashed && !a.has_value && a.name.size() > 1 && !matches_full_name<T>(a.name)) {
                if (is_flag_bundle<T>(a.name)) {
                    for (size_t f{0}; f < a.name.size(); ++f) {
                        ArgAttributes flag{a};
                        flag.name = a.name.substr(f, 1);
                        attr_array.push_back(flag);
                    }
                    continue;
                }
                if (option_takes_value<T>(a.name.substr(0, 1), true)) {
                    // try attached value split (e.g., -ofile for -o file)
                    a.value = a.name.substr(1, a.name.size() - 1);
                    a.name = a.name.substr(0, 1);
                    a.has_value = true;
                }
            }
            if ((a.dashed || a.double_dashed) && !a.has_value && option_takes_value<T>(a.name, a.dashed) && i + 1 <
                argc) {
                a.value = argv[++i];
                a.has_value = true;
            }
            attr_array.push_back(a);
        }
        return attr_array;
    }

    template<std::meta::info M>
    inline constexpr std::string_view snake_name_v = std::meta::identifier_of(M);

    template<std::meta::info M>
    inline constexpr std::string_view kebab_name_v = [] {
        std::string s(std::meta::identifier_of(M));
        std::ranges::replace(s, '_', '-');
        return std::string_view(std::define_static_string(s), s.size());
    }();

    template<std::meta::info M>
    constexpr bool does_match_name(std::string_view name,
                                   const std::optional<Opt> &opt,
                                   const Settings &settings) {
        if (name.size() == 1 && opt.has_value() && name[0] == opt->short_name)
            return true;
        if (name == snake_name_v<M>)
            return true;
        if (settings.allow_kebab_casing && name == kebab_name_v<M>)
            return true;
        if (settings.allow_aliases && matches_alias<M>(name))
            return true;
        return false;
    }

    template<typename T>
    ParsedArgs<T> parse(int argc, char **argv, T initial = {}, Settings settings = {}) {
        static_assert(no_required_optionals<T>(),
                      "a Required field cannot have a std::optional type; drop one of the two");
        T out{std::move(initial)};
        bool help_requested{};
        std::vector<ParseError> errors{};
        static constexpr auto members = std::define_static_array(
            std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        size_t next_positional{0};
        ArgContext ctx{};
        const auto attr_array = get_arg_attrs<T>(argc, argv, settings);
        std::array<bool, members.size()> fields_set{};

        for (const auto &attrs : attr_array) {
            bool matched{false};
            template for (constexpr auto m : members) {
                using M = [:std::meta::type_of(m):];
                constexpr auto pos = positional_index_of<T>(m);
                if (attrs.is_positional || ctx.separator_seen) {
                    if constexpr (pos.has_value()) {
                        if (!matched && next_positional == *pos) {
                            matched = true;
                            next_positional++;
                            out.[:m:] = from_string<M>(attrs.name);
                            fields_set[member_index_of<T>(m)] = true;
                        }
                    }
                } else if (attrs.is_separator) {
                    ctx.separator_seen = true;
                } else if (attrs.dashed || attrs.double_dashed) {
                    constexpr auto opt = opt_of(m);
                    constexpr bool named = is_named_option<T>(m);
                    const bool matching_string = named && does_match_name<m>(attrs.name, opt, settings);
                    if (!matched && matching_string) {
                        matched = true;
                        if constexpr (is_flag_type(^^M)) {
                            if (attrs.has_value) {
                                throw std::invalid_argument(
                                    std::format("option does not take a value: {}", attrs.name));
                            }
                            out.[:m:] = true;
                            fields_set[member_index_of<T>(m)] = true;
                        } else {
                            if (!attrs.has_value) {
                                throw std::invalid_argument(std::format("missing value for option: {}", attrs.name));
                            }
                            out.[:m:] = from_string<M>(attrs.value);
                            fields_set[member_index_of<T>(m)] = true;
                        }
                    }
                }
            }
            if (!matched) {
                throw std::invalid_argument(std::format("unrecognized argument: {}", attrs.name));
            }
        }

        std::vector<std::string_view> missing_fields;
        template for (constexpr auto m : members) {
            if constexpr (is_required(m)) {
                if (!fields_set[member_index_of<T>(m)]) {
                    missing_fields.push_back(std::string_view{std::meta::identifier_of(m)});
                }
            }
        }
        if (!missing_fields.empty()) {
            throw std::invalid_argument(std::format("Required fields not specified: {}", missing_fields));
        }

        return ParsedArgs<T>{out, help_requested, errors};
    }

    template<typename T>
    T immediate_parse(const int argc, char **argv, T initial = {}, const Settings settings = {}) {
        auto result = parse<T>(argc, argv, std::move(initial), settings);
        return std::move(result).value();
    }
}
