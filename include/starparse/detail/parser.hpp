#pragma once

#include <algorithm>
#include <format>
#include <optional>
#include <string_view>
#include <meta>
#include <string>
#include <array>
#include <vector>

#include <starparse/detail/annotations.hpp>
#include <starparse/detail/settings.hpp>
#include <starparse/detail/utilities.hpp>
#include <starparse/detail/errors.hpp>

namespace StarParse::detail::Parser {
    using namespace StarParse::detail::Utilities;

    struct ArgAttributes {
        int argv_index{};
        bool dashed{};
        bool double_dashed{};
        bool is_help{};
        bool is_version{};
        bool is_positional{};
        bool is_separator{};
        bool has_value{};
        std::string_view name{};
        std::string_view value{};

        explicit ArgAttributes(std::string_view argument,
                               const int index,
                               const bool separator_seen) : argv_index(index) {
            if (separator_seen || !argument.starts_with('-') || (argument.starts_with('-') && argument.size() == 1)) {
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
            } else if (double_dashed && name == "version") {
                is_version = true;
            }
        }
    };

    template<typename T>
    bool matches_full_name(std::string_view name, const Settings &settings) {
        static constexpr auto members = std::define_static_array(
            std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        bool found{false};
        template for (constexpr auto m : members) {
            constexpr bool named = is_named_option<T>(m);
            if (named && does_match_name<m>(name, opt_of(m), settings)) {
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
                        if (Utilities::matches_alias<m>(std::string_view{&c, 1})) {
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

    template<typename T>
    class ParsedArgs {
    public:
        ParsedArgs(T out, const bool show_help, const bool show_version,
                   std::vector<ParseError> &errors) : out_(std::move(out)),
                                                      show_help_(show_help), show_version_(show_version),
                                                      errors_(std::move(errors)) {}

        T &&value() && {
            return std::move(out_);
        }

        [[nodiscard]] std::span<const ParseError> errors() const {
            return errors_;
        }

        [[nodiscard]] bool help_requested() const {
            return show_help_;
        }

        [[nodiscard]] bool version_requested() const {
            return show_version_;
        }

        [[nodiscard]] static std::string version() {
            constexpr auto program = program_of(^^T);
            if constexpr (program.has_value()) {
                return std::format("{} version {}", program->name, program->version);
            } else {
                return "No version information.";
            }
        }

        [[nodiscard]] std::string help() const {
            static constexpr auto members = std::define_static_array(
                std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
            constexpr auto program = program_of(^^T);
            constexpr std::string_view program_name =
                    program.has_value() ? std::string_view{program->name} : std::string_view{"program"};

            struct ArgumentRow {
                size_t index{};
                std::string_view name;
                std::string description;
                bool required{};
            };
            struct OptionRow {
                std::string invocation;
                std::string description;
            };
            std::vector<ArgumentRow> arguments;
            std::vector<OptionRow> options;

            template for (constexpr auto m : members) {
                using M = [:std::meta::type_of(m):];
                constexpr auto opt = opt_of(m);
                constexpr auto pos = positional_of(m);
                constexpr auto name = std::string_view{std::meta::identifier_of(m)};
                constexpr bool required = is_required(m);

                std::string description;
                if constexpr (opt.has_value()) {
                    if (opt->help_ != nullptr) description = opt->help();
                }
                if constexpr (pos.has_value()) {
                    if (pos->help_ != nullptr) description = pos->help();
                    if constexpr (required) {
                        description += description.empty() ? "(required)" : " (required)";
                    }
                }

                if constexpr (pos.has_value()) {
                    arguments.push_back({pos->index, name, std::move(description), required});
                } else if constexpr (is_named_option<T>(m)) {
                    std::string invocation{"    "};
                    if constexpr (opt.has_value()) {
                        if constexpr (opt->short_name != 0) {
                            invocation = std::format("-{}, ", opt->short_name);
                        }
                    }
                    invocation += std::format("--{}", name);
                    for (const char *alias : alias_names<m>()) {
                        const std::string_view a{alias};
                        invocation += std::format(", {}{}", a.size() == 1 ? "-" : "--", a);
                    }
                    if constexpr (!is_flag_type(^^M)) {
                        invocation += " <value>";
                    }
                    options.push_back({std::move(invocation), std::move(description)});
                }
            }
            std::ranges::sort(arguments, {}, &ArgumentRow::index);
            options.push_back({"    --help", "Show this help message"});
            options.push_back({"    --version", "Show version information"});

            std::string usage = std::format("Usage: {} [options]", program_name);
            for (const auto &argument : arguments) {
                usage += argument.required
                             ? std::format(" <{}>", argument.name)
                             : std::format(" [{}]", argument.name);
            }

            size_t column{0};
            for (const auto &argument : arguments) column = std::max(column, argument.name.size());
            for (const auto &option : options) column = std::max(column, option.invocation.size());
            column += 2;

            std::string text;
            if constexpr (program.has_value()) {
                if (program->description != nullptr && *program->description != '\0') {
                    text += std::format("{} - {}\n\n", program_name, program->description);
                } else {
                    text += std::format("{}\n\n", program_name);
                }
            }
            text += usage;
            text += '\n';
            if (!arguments.empty()) {
                text += "\nArguments:\n";
                for (const auto &argument : arguments) {
                    if (argument.description.empty()) text += std::format("  {}\n", argument.name);
                    else text += std::format("  {:<{}}{}\n", argument.name, column, argument.description);
                }
            }
            text += "\nOptions:\n";
            for (const auto &option : options) {
                if (option.description.empty()) text += std::format("  {}\n", option.invocation);
                else text += std::format("  {:<{}}{}\n", option.invocation, column, option.description);
            }
            return text;
        }

    private:
        T out_{};
        bool show_help_{};
        bool show_version_{};
        std::vector<ParseError> errors_;
    };

    template<typename T>
    std::vector<ArgAttributes> get_arg_attrs(const int argc, char **argv, const Settings &settings) {
        std::vector<ArgAttributes> attr_array;
        attr_array.reserve(argc - 1);
        bool separator_seen{false};
        for (int i{1}; i < argc; ++i) {
            ArgAttributes a{argv[i], i, separator_seen};
            if (a.is_separator) {
                separator_seen = true;
                continue;
            }
            if (separator_seen) {
                // post-separator, all args are positional
                a.is_positional = true;
            } else if (a.dashed && !a.has_value && a.name.size() > 1 && !matches_full_name<T>(a.name, settings)) {
                if (is_flag_bundle<T>(a.name)) {
                    for (auto f{0uz}; f < a.name.size(); ++f) {
                        ArgAttributes flag{a};
                        flag.name = a.name.substr(f, 1);
                        attr_array.push_back(flag);
                    }
                    continue;
                }
                if (Utilities::option_takes_value<T>(a.name.substr(0, 1), true, settings)) {
                    // try attached value split (e.g., -ofile for -o file)
                    a.value = a.name.substr(1, a.name.size() - 1);
                    a.name = a.name.substr(0, 1);
                    a.has_value = true;
                }
            }
            if ((a.dashed || a.double_dashed) && !a.has_value && Utilities::option_takes_value<T>(
                    a.name, a.dashed, settings) && i + 1 < argc) {
                a.value = argv[++i];
                a.has_value = true;
            }
            attr_array.push_back(a);
        }
        return attr_array;
    }

    template<typename T>
    ParsedArgs<T> parse(int argc, char **argv, T initial = {}, Settings settings = {}) {
        static_assert(Utilities::no_positional_containers<T>(),
                      "cannot use Positional in combination with a container");
        static_assert(Utilities::no_required_optionals<T>(),
                      "a Required field cannot have a std::optional type; drop one of the two");
        if (argc == 0) {
            std::vector<ParseError> errors;
            return ParsedArgs<T>{initial, false, false, errors};
        }
        T out{std::move(initial)};
        bool help_requested{}, version_requested{};
        std::vector<ParseError> errors{};
        static constexpr auto members = std::define_static_array(
            std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        size_t next_positional{0};
        ArgContext ctx{};
        const auto attr_array = get_arg_attrs<T>(argc, argv, settings);
        std::array<size_t, members.size()> fields_set{};

        for (const auto &attrs : attr_array) {
            bool matched{false};
            if (attrs.is_help) {
                help_requested = true;
                break;
            }
            if (attrs.is_version) {
                version_requested = true;
                break;
            }
            template for (constexpr auto m : members) {
                using M = [:std::meta::type_of(m):];
                constexpr auto idx = member_index_of<T>(m);
                constexpr auto pos = positional_index_of<T>(m);
                if (attrs.is_positional || ctx.separator_seen) {
                    if constexpr (pos.has_value()) {
                        if (!matched && next_positional == *pos) {
                            matched = true;
                            next_positional++;
                            assign_from_string<m>(out.[:m:], attrs.name, attrs.argv_index, fields_set[idx], errors, settings);
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
                                assign_from_string<m>(out.[:m:], attrs.value, attrs.argv_index, fields_set[idx], errors, settings);
                            } else {
                                out.[:m:] = true;
                            }
                            fields_set[idx] = 1;
                        } else {
                            if (!attrs.has_value) {
                                errors.push_back({
                                    .kind = ErrorKind::MISSING_VALUE, .token{}, .option = attrs.name,
                                    .argv_index = attrs.argv_index
                                });
                                continue;
                            }
                            assign_from_string<m>(out.[:m:], attrs.value, attrs.argv_index, fields_set[idx], errors, settings);
                        }
                    }
                }
            }
            if (!matched) {
                errors.push_back({
                    .kind = ErrorKind::UNKNOWN_OPTION, .token = attrs.name, .option{},
                    .argv_index = attrs.argv_index
                });
            }
        }

        std::vector<std::string_view> missing_fields;
        template for (constexpr auto m : members) {
            const size_t index = member_index_of<T>(m);
            if constexpr (is_required(m)) {

                if (fields_set[index] == 0) {
                    const auto field_name = std::string_view{std::meta::identifier_of(m)};
                    errors.push_back({
                        .kind = ErrorKind::MISSING_REQUIRED, .token{}, .option = field_name,
                        .argv_index = index
                    });
                }
            } else if constexpr (is_array(m)) {
                if (fields_set[index] < std::meta::tuple_size(m)) {
                    const auto field_name = std::string_view{std::meta::identifier_of(m)};
                    errors.push_back({
                        .kind = ErrorKind::MISSING_VALUE, .token{}, .option = field_name,
                        .argv_index = index
                    });
                }
            }
        }

        return ParsedArgs<T>{out, help_requested, version_requested, errors};
    }
}
