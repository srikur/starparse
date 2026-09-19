#pragma once

#include <ranges>
#include <algorithm>
#include <format>
#include <optional>
#include <string_view>
#include <meta>
#include <string>
#include <array>
#include <vector>
#include <memory>
#include <numeric>

#include <starparse/detail/annotations.hpp>
#include <starparse/detail/settings.hpp>
#include <starparse/detail/utilities.hpp>
#include <starparse/detail/errors.hpp>

#include "assertions.hpp"

namespace StarParse::detail::Parser {
    using namespace StarParse::detail::Utilities;
    using namespace StarParse::detail::Assertions;

    struct ArgAttributes {
        size_t argv_index{};
        bool dashed{};
        bool double_dashed{};
        bool is_help{};
        bool is_version{};
        bool is_positional{};
        bool is_separator{};
        bool is_subcommand{};
        bool has_value{};
        std::string_view name{};
        std::string_view value{};

        explicit ArgAttributes(std::string_view argument,
                               const size_t index,
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
    bool is_flag(const char c, const Settings &settings) {
        static constexpr auto members = std::define_static_array(
            std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        template for (constexpr auto m : members) {
            using M = [:std::meta::type_of(m):];
            if constexpr (is_flag_type(^^M) && is_named_option<T>(m)) {
                if (does_match_name<m>(std::string_view{&c, 1}, opt_of(m), settings)) {
                    return true;
                }
            }
        }
        return false;
    }

    template<typename T>
    bool is_flag_bundle(const std::string_view name, const Settings &settings) {
        if (name.size() < 2) {
            return false;
        }
        for (auto i{0uz}; i < name.size(); ++i) {
            // as soon as first non-bool option found, assume rest of string is value
            if (Utilities::option_takes_value<T>(name.substr(i, 1), true, settings)) {
                return true;
            }
            if (!is_flag<T>(name[i], settings)) return false;
        }
        return true;
    }

    struct ArgContext {
        bool separator_seen{};
    };

    template<typename T>
    std::string format_help(std::span<const size_t> command_path, const std::string &command_name,
                            std::string usage, const bool allow_aliases) {
        static constexpr auto members = std::define_static_array(
            std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        constexpr auto program = program_of(^^T);

        struct ArgumentRow {
            size_t index{};
            std::string_view name;
            std::string description;
            bool required{};
        };
        struct HelpRow {
            std::string invocation;
            std::string description;
        };
        std::vector<ArgumentRow> arguments;
        std::vector<HelpRow> options;
        std::vector<HelpRow> commands;

        template for (constexpr auto m : members) {
            using M = [:std::meta::type_of(m):];
            constexpr auto opt = opt_of(m);
            constexpr auto pos = positional_of(m);
            constexpr auto position = positional_index_of<T>(m);
            constexpr auto name = std::string_view{std::meta::identifier_of(m)};
            constexpr bool required = is_required(m);

            std::string description;
            if constexpr (opt.has_value()) {
                if (opt->help_ != nullptr) description = opt->help();
            }
            if constexpr (pos.has_value()) {
                if (pos->help_ != nullptr) description = pos->help();
            }
            if constexpr (required) {
                description += description.empty() ? "(required)" : " (required)";
            }

            if constexpr (is_subcommand(m)) {
                using Child = [:value_type_of(^^M):];
                constexpr auto child_program = program_of(^^Child);
                if constexpr (child_program.has_value()) {
                    if (child_program->description != nullptr) description = child_program->description;
                }
                std::string invocation{name};
                if (allow_aliases) {
                    for (const char *alias : alias_names<m>()) {
                        invocation += std::format(", {}", alias);
                    }
                }
                commands.push_back({std::move(invocation), std::move(description)});
            } else {
                if constexpr (position.has_value()) {
                    arguments.push_back({*position, name, description, required});
                }
                if constexpr (is_named_option<T>(m)) {
                    std::string invocation{"    "};
                    if constexpr (opt.has_value()) {
                        if constexpr (opt->short_name != 0) {
                            invocation = std::format("-{}, ", opt->short_name);
                        }
                    }
                    invocation += std::format("--{}", name);
                    if (allow_aliases) {
                        for (const char *alias : alias_names<m>()) {
                            const std::string_view a{alias};
                            invocation += std::format(", {}{}", a.size() == 1 ? "-" : "--", a);
                        }
                    }
                    if constexpr (!is_flag_type(^^M)) {
                        invocation += " <value>";
                    }
                    options.push_back({std::move(invocation), std::move(description)});
                }
            }
        }
        std::ranges::sort(arguments, {}, &ArgumentRow::index);
        options.push_back({"    --help", "Show this help message"});
        options.push_back({"    --version", "Show version information"});

        usage += " [options]";
        for (const auto &argument : arguments) {
            usage += argument.required
                         ? std::format(" <{}>", argument.name)
                         : std::format(" [{}]", argument.name);
        }

        if (!command_path.empty()) {
            template for (constexpr auto m : members) {
                if constexpr (is_subcommand(m)) {
                    if (command_path.front() == member_index_of<T>(m)) {
                        using Child = [:value_type_of(std::meta::type_of(m)):];
                        constexpr auto name = std::meta::identifier_of(m);
                        return format_help<Child>(command_path.subspan(1),
                                                  std::format("{} {}", command_name, name),
                                                  std::format("{} {}", usage, name), allow_aliases);
                    }
                }
            }
        }
        if (!commands.empty()) usage += " [command]";

        size_t column{0};
        for (const auto &argument : arguments) column = std::max(column, argument.name.size());
        for (const auto &option : options) column = std::max(column, option.invocation.size());
        for (const auto &command : commands) column = std::max(column, command.invocation.size());
        column += 2;

        std::string text;
        if constexpr (program.has_value()) {
            if (program->description != nullptr && *program->description != '\0') {
                text += std::format("{} - {}\n\n", command_name, program->description);
            } else {
                text += std::format("{}\n\n", command_name);
            }
        }
        text += std::format("Usage: {}\n", usage);
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
        if (!commands.empty()) {
            text += "\nCommands:\n";
            for (const auto &command : commands) {
                if (command.description.empty()) text += std::format("  {}\n", command.invocation);
                else text += std::format("  {:<{}}{}\n", command.invocation, column, command.description);
            }
        }
        return text;
    }

    template<typename T>
    class ParsedArgs {
    public:
        ParsedArgs(T out, const bool show_help, const bool show_version,
                   std::vector<ParseError> &errors, std::vector<size_t> command_path = {},
                   const bool allow_aliases = true) : out_(std::move(out)),
                                                      show_help_(show_help), show_version_(show_version),
                                                      errors_(std::move(errors)),
                                                      command_path_(std::move(command_path)), allow_aliases_(allow_aliases) {}

        T &&value() && {
            return std::move(out_);
        }

        explicit operator bool() const {
            return errors_.empty();
        }

        T &operator*() {
            return out_;
        }

        const T &operator*() const {
            return out_;
        }

        [[nodiscard]] auto operator->(this auto &self) noexcept {
            return std::addressof(self.out_);
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

        [[nodiscard]] std::string version() const {
            constexpr auto program = program_of(^^T);
            if constexpr (program.has_value()) {
                return std::format("{} version {}", program->name, program->version);
            } else {
                return "No version information.";
            }
        }

        [[nodiscard]] std::string help() const {
            constexpr auto program = program_of(^^T);
            const std::string program_name = program.has_value() ? program->name : "program";
            return format_help<T>(command_path_, program_name, program_name, allow_aliases_);
        }

        [[nodiscard]] std::string error_message() const {
            if (errors_.empty()) { return ""; }
            const auto error_strings = errors_
                                       | std::views::transform(
                                           [](const ParseError &error) { return error.to_string(); })
                                       | std::ranges::to<std::vector<std::string> >();
            return std::accumulate(error_strings.begin() + 1, error_strings.end(), error_strings[0],
                                   [](std::string a, const std::string &b) {
                                       return std::move(a) + '\n' + b;
                                   });
        }

    private:
        T out_{};
        bool show_help_{};
        bool show_version_{};
        std::vector<ParseError> errors_;
        std::vector<size_t> command_path_;
        bool allow_aliases_{};
    };

    template<typename T>
    bool takes_next_value(const ArgAttributes &option, const std::string_view next, const Settings &settings) {
        if (!Utilities::option_is_count<T>(option.name, option.dashed, settings)) return true;
        if (next.size() > 1 && next.starts_with('-') && !(next[1] >= '0' && next[1] <= '9')) return false;

        static constexpr auto members = std::define_static_array(
            std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        template for (constexpr auto m : members) {
            if constexpr (is_subcommand(m)) {
                if (does_match_name<m>(next, std::nullopt, settings, false)) return false;
            }
        }
        return true;
    }

    template<typename T>
    void get_arg_attrs_into(std::span<const std::string_view> args, const Settings &settings, size_t &i, bool &separator_seen,
                            std::vector<ArgAttributes> &attrs) {
        static constexpr auto members = std::define_static_array(
            std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        for (; i < args.size(); ++i) {
            ArgAttributes a{args[i], i + 1, separator_seen};
            if (a.is_separator) {
                separator_seen = true;
                continue;
            }
            if (separator_seen) {
                // post-separator, all args are positional
                a.is_positional = true;
            } else if (a.dashed && !a.has_value && a.name.size() > 1 && !matches_full_name<T>(a.name, settings)) {
                if (is_flag_bundle<T>(a.name, settings)) {
                    for (auto f{0uz}; f < a.name.size(); ++f) {
                        ArgAttributes flag{a};
                        flag.name = a.name.substr(f, 1);
                        if (Utilities::option_takes_value<T>(flag.name, true, settings)) {
                            flag.value = a.name.substr(f + 1);
                            flag.has_value = !flag.value.empty();
                            if (!flag.has_value && i + 1 < args.size() && takes_next_value<T>(flag, args[i + 1], settings)) {
                                flag.value = args[++i];
                                flag.has_value = true;
                            }
                            attrs.push_back(flag);
                            break;
                        }
                        attrs.push_back(flag);
                    }
                    continue;
                }
            }
            if ((a.dashed || a.double_dashed) && !a.has_value && Utilities::option_takes_value<T>(
                    a.name, a.dashed, settings) && i + 1 < args.size() && takes_next_value<T>(a, args[i + 1], settings)) {
                a.value = args[++i];
                a.has_value = true;
            }
            // check for subcommand
            if (a.is_positional && !separator_seen) {
                template for (constexpr auto m : members) {
                    if constexpr (is_subcommand(m)) {
                        using M = [:std::meta::type_of(m):];
                        static_assert(is_optional(^^M));
                        using Child = [:value_type_of(^^M):];

                        if (does_match_name<m>(a.name, std::nullopt, settings, false)) {
                            a.is_subcommand = true;
                            a.is_positional = false;
                            attrs.push_back(a);
                            i++;
                            get_arg_attrs_into<Child>(args, settings, i, separator_seen, attrs);
                            return;
                        }
                    }
                }
            }
            attrs.push_back(a);
        }
    }

    template<typename T>
    std::vector<ArgAttributes> get_arg_attrs(const std::span<const std::string_view> args, const Settings &settings) {
        std::vector<ArgAttributes> attr_array;
        attr_array.reserve(args.size());
        bool separator_seen{false};
        auto index{0uz};
        get_arg_attrs_into<T>(args, settings, index, separator_seen, attr_array);
        return attr_array;
    }

    template<typename T>
    void check_assertions() {
        static_assert(Assertions::check_annotation_placement<T>(),
                      "misplaced annotation: Program belongs on the argument type; enum values only accept Alias");
        static_assert(Assertions::no_annotations_on_ignored_fields<T>(),
                      "annotated fields need Opt, Positional, or Subcommand when another field uses Opt or Positional");
        static_assert(Assertions::no_duplicate_annotations<T>(),
                      "annotations other than Alias may only appear once on an entity");
        static_assert(Assertions::check_name_values<T>(),
                      "invalid option spelling: aliases must be nonempty, contain no whitespace or '=', and not start with '-'; "
                      "short names cannot be whitespace, '-' or '='");
        static_assert(Assertions::no_positional_containers<T>(),
                      "cannot use Positional in combination with a container");
        static_assert(Assertions::no_required_optionals<T>(),
                      "a Required field cannot have a std::optional type; drop one of the two");
        static_assert(Assertions::no_duplicate_short_names<T>(), "two fields cannot have duplicate Opt short names");
        static_assert(Assertions::check_positional_indices<T>(),
                      "Positional indices must be unique, begin at 0, and increment contiguously");
        static_assert(Assertions::no_duplicate_validators<T>(),
                      "only one annotation among Min, Max, Range, Choices, or Validator can be applied to a single field");
        static_assert(Assertions::no_scalar_separators<T>(), "the Separator annotation cannot be applied to scalar fields");
        static_assert(Assertions::check_ranges<T>(), "invalid Range annotation for specified type");
        static_assert(Assertions::check_alias_collisions<T>(),
                      "option names, short names, or aliases collide, or use reserved help/version names (including case and kebab spellings)");
        static_assert(Assertions::check_subcommand_collisions<T>(),
                      "subcommand names or aliases collide (including case and kebab spellings)");
        static_assert(Assertions::check_enum_alias_collisions<T>(),
                      "enum aliases or enumerator names collide (including case and kebab spellings)");
    }

    struct ParseState {
        bool help_requested{false};
        bool version_requested{false};
        std::vector<ParseError> errors{};
        std::vector<size_t> command_path{};
    };

    template<typename T>
    void parse_into(std::span<const ArgAttributes> attributes, T &out, const Settings &settings, ParseState &state) {
        static constexpr auto members = std::define_static_array(
            std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        check_assertions<T>();
        std::array<size_t, members.size()> fields_set{};
        size_t next_positional{0uz};
        auto &errors = state.errors;
        for (auto i{0uz}; i < attributes.size(); ++i) {
            const auto &attrs = attributes[i];
            bool matched{false}, entered_child{false};

            if (attrs.is_help) {
                state.help_requested = true;
                break;
            }
            if (attrs.is_version) {
                state.version_requested = true;
                break;
            }
            template for (constexpr auto m : members) {
                using M = [:std::meta::type_of(m):];
                constexpr auto idx = member_index_of<T>(m);
                constexpr auto pos = positional_index_of<T>(m);
                if constexpr (is_subcommand(m)) {
                    if (attrs.is_subcommand && !matched && does_match_name<m>(attrs.name, std::nullopt, settings, false)) {
                        using M = [:std::meta::type_of(m):];
                        using Child = [:value_type_of(^^M):];
                        auto &child = out.[:m:];
                        if (!child) child.emplace();
                        matched = true;
                        ++fields_set[idx];

                        state.command_path.push_back(idx);
                        parse_into<Child>(attributes.subspan(i + 1), *child, settings, state);
                        entered_child = true;
                    }
                } else {
                    if (attrs.is_positional) {
                        if constexpr (pos.has_value()) {
                            if (!matched && next_positional == *pos) {
                                matched = true;
                                next_positional++;
                                assign_from_string<m>(out.[:m:], attrs.name, attrs.argv_index, fields_set[idx], errors,
                                                      settings);
                            }
                        }
                    } else if (attrs.dashed || attrs.double_dashed) {
                        constexpr auto opt = opt_of(m);
                        constexpr bool named = is_named_option<T>(m);
                        const bool matching_string = named && does_match_name<m>(attrs.name, opt, settings);
                        if (!matched && matching_string) {
                            matched = true;
                            if constexpr (is_flag_type(^^M)) {
                                if (attrs.has_value) {
                                    assign_from_string<m>(out.[:m:], attrs.value, attrs.argv_index, fields_set[idx], errors,
                                                          settings);
                                } else {
                                    out.[:m:] = !settings.autogenerate_negations || !attrs.name.starts_with("no-");
                                }
                                fields_set[idx] = 1;
                            } else {
                                if (!attrs.has_value) {
                                    if constexpr (is_count_type(^^M)) {
                                        if (settings.allow_repeated_counts) {
                                            increment_count<m>(out.[:m:], attrs.name, attrs.argv_index, errors, settings);
                                            ++fields_set[idx];
                                            continue;
                                        }
                                    }
                                    errors.push_back({
                                        .kind = ErrorKind::MISSING_VALUE, .current_argument = attrs.name,
                                        .argv_index = attrs.argv_index
                                    });
                                    continue;
                                }
                                assign_from_string<m>(out.[:m:], attrs.value, attrs.argv_index, fields_set[idx], errors,
                                                      settings);
                            }
                        }
                    }
                }
            }
            if (entered_child) break;
            if (!matched) {
                errors.push_back({
                    .kind = ErrorKind::UNKNOWN_OPTION, .input_value = attrs.name,
                    .argv_index = attrs.argv_index
                });
            }
        }

        if (state.help_requested) return;
        template for (constexpr auto m : members) {
            const size_t index = member_index_of<T>(m);
            if constexpr (is_required(m)) {
                if (fields_set[index] == 0) {
                    const auto field_name = std::string_view{std::meta::identifier_of(m)};
                    state.errors.push_back({
                        .kind = ErrorKind::MISSING_REQUIRED,
                        .current_argument = std::optional{field_name},
                        .argv_index = index
                    });
                }
            } else if constexpr (is_array(m)) {
                if (fields_set[index] < std::meta::tuple_size(m)) {
                    const auto field_name = std::string_view{std::meta::identifier_of(m)};
                    state.errors.push_back({
                        .kind = ErrorKind::MISSING_VALUE,
                        .current_argument = std::optional{field_name},
                        .argv_index = index
                    });
                }
            }
        }
    }

    template<typename T>
    ParsedArgs<T> parse(std::span<const std::string_view> args, T initial = {}, Settings settings = {}) {
        T out{std::move(initial)};
        ParseState state{};
        const auto attr_array = get_arg_attrs<T>(args, settings);

        parse_into<T>(attr_array, out, settings, state);
        return ParsedArgs<T>{std::move(out), state.help_requested, state.version_requested, state.errors};
    }
}
