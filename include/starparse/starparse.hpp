#pragma once

#include <algorithm>
#include <optional>
#include <string_view>
#include <meta>
#include <ranges>
#include <string>
#include <stdexcept>
#include <print>
#include <array>
#include <vector>

#include <starparse/detail/annotations.h>
#include <starparse/detail/settings.h>
#include <starparse/detail/utilities.h>

namespace StarParse {
    using namespace std::literals;
    using namespace detail::StarParse::Annotations;
    using detail::StarParse::Settings;
    using namespace detail::StarParse::Utilities;

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
                                out.[:m:] = from_string<M>(attrs.value);
                            } else {
                                out.[:m:] = true;
                            }
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
