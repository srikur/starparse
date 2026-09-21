#pragma once

#include <concepts>
#include <optional>
#include <string_view>
#include <meta>
#include <string>
#include <stdexcept>
#include <expected>
#include <charconv>
#include <type_traits>
#include <array>
#include <chrono>
#include <span>
#include <vector>
#include <utility>
#include <limits>

#include <starparse/detail/settings.hpp>
#include <starparse/detail/annotations.hpp>
#include <starparse/detail/errors.hpp>

namespace StarParse::detail::Utilities {
    consteval std::vector<const char *> alias_name_list(const std::meta::info m) {
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

    inline bool iequals(const std::string_view a, const std::string_view b) {
        if (a.length() != b.length()) {
            return false;
        }
        return std::equal(a.begin(), a.end(), b.begin(), [](const unsigned char ac, const unsigned char bc) {
            return std::tolower(ac) == std::tolower(bc);
        });
    }

    template<std::meta::info M>
    bool matches_alias(const std::string_view name, bool allow_case_insensitivity = false) {
        static constexpr auto aliases = std::define_static_array(alias_name_list(M));
        for (const char *alias : aliases) {
            if (name == std::string_view{alias} || (allow_case_insensitivity && iequals(name, std::string_view{alias})))
                return true;
        }
        return false;
    }

    consteval std::optional<Opt> opt_of(const std::meta::info m) {
        for (const std::meta::info a : std::meta::annotations_of(m)) {
            if (std::meta::dealias(std::meta::remove_cv(std::meta::type_of(a))) == std::meta::dealias(^^Opt)) {
                return std::meta::extract<Opt>(a);
            }
        }
        return std::nullopt;
    }

    template<std::meta::info M>
    std::span<const char *const> alias_names() {
        static constexpr auto aliases = std::define_static_array(alias_name_list(M));
        return aliases;
    }

    consteval bool is_optional(std::meta::info r) {
        r = std::meta::dealias(r);
        return std::meta::has_template_arguments(r) && std::meta::template_of(r) == ^^std::optional;
    }

    consteval bool is_subcommand(const std::meta::info m) {
        for (const auto a : std::meta::annotations_of(m)) {
            if (std::meta::dealias(std::meta::remove_cv(std::meta::type_of(a))) == std::meta::dealias(^^detail::Subcommand_)) {
                return true;
            }
        }
        return false;
    }

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

    consteval bool is_count_type(std::meta::info r) {
        r = std::meta::dealias(std::meta::remove_cv(r));
        if (is_optional(r)) r = std::meta::dealias(value_type_of(r));
        return std::meta::is_integral_type(r) && r != (^^bool) &&
               !std::meta::extract<bool>(std::meta::substitute(^^is_char_v, {r}));
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
    constexpr bool check_snake_case(const std::string_view name, const Settings &settings) {
        return name == snake_name_v<M> || (settings.allow_case_insensitivity && iequals(name, snake_name_v<M>));
    }

    template<std::meta::info M>
    constexpr bool check_kebab_case(const std::string_view name, const Settings &settings) {
        return settings.allow_kebab_casing && (name == kebab_name_v<M> || (settings.allow_case_insensitivity && iequals(name, kebab_name_v<M>)));
    }

    template<std::meta::info M>
    constexpr bool does_match_name(std::string_view name,
                                   const std::optional<Opt> &opt,
                                   const Settings &settings,
                                   const bool allow_short = true) {
        if (allow_short && name.size() == 1 && opt.has_value() && name[0] == opt->short_name)
            return true;
        if (check_snake_case<M>(name, settings) || check_kebab_case<M>(name, settings))
            return true;
        if (settings.allow_aliases && Utilities::matches_alias<M>(name, settings.allow_case_insensitivity))
            return true;
        if (settings.autogenerate_negations && name.starts_with("no-") && is_flag_type(std::meta::type_of(M))) {
            std::string_view negated{name};
            negated.remove_prefix(3);
            return check_snake_case<M>(negated, settings) || check_kebab_case<M>(negated, settings);
        }
        return false;
    }

    consteval bool is_specialization_of(std::meta::info type, const std::meta::info templ) {
        type = std::meta::dealias(type);
        return std::meta::has_template_arguments(type) && std::meta::template_of(type) == templ;
    }

    consteval bool is_vector(const std::meta::info r) { return is_specialization_of(r, ^^std::vector); }
    consteval bool is_array(const std::meta::info r) { return is_specialization_of(r, ^^std::array); }
    consteval bool is_container(const std::meta::info m) { return is_vector(m) || is_array(m); }

    constexpr std::expected<bool, ParseError> bool_from_string(const std::string_view s) {
        using namespace std::literals;
        constexpr std::array true_values{"yes"sv, "1"sv, "on"sv, "true"sv, "t"sv};
        constexpr std::array false_values{"no"sv, "0"sv, "off"sv, "false"sv, "f"sv};
        if (std::ranges::any_of(true_values, [&](auto value) { return iequals(s, value); })) return true;
        if (std::ranges::any_of(false_values, [&](auto value) { return iequals(s, value); })) return false;
        return std::unexpected(ParseError{.kind = ErrorKind::INVALID_VALUE, .input_value = s});
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

    template<typename M>
    std::expected<M, ParseError> from_string(std::string_view s, const size_t index, const Settings &settings) {
        if constexpr (is_optional(^^M)) {
            using T = [:value_type_of(^^M):];
            if (auto result = from_string<T>(s, index, settings)) return M{*result};
            else return std::unexpected(result.error());
        } else if constexpr (std::same_as<M, bool>) {
            if (auto result = bool_from_string(s)) return M{*result};
            else return std::unexpected(result.error());
        } else if constexpr (std::constructible_from<M, std::string_view>) {
            return M{s};
        } else if constexpr (is_char_v<M>) {
            return s.empty() ? M{0} : M{s[0]};
        } else if constexpr (std::is_arithmetic_v<M>) {
            M v{};
            auto [pointer, error_code] = std::from_chars(s.data(), s.data() + s.size(), v);
            if (error_code != std::errc{} || pointer != s.data() + s.size()) {
                return std::unexpected(ParseError{
                    .kind = ErrorKind::INVALID_VALUE,
                    .input_value = s,
                    .current_argument = std::optional{std::meta::display_string_of(^^M)},
                    .argv_index = index
                });
            }
            return v;
        } else if constexpr (std::is_enum_v<M>) {
            std::optional<M> parsed;
            template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^M))) {
                if (!parsed.has_value() && does_match_name<e>(s, std::nullopt, settings)) {
                    parsed = [:e:];
                }
            }
            if (!parsed)
                return std::unexpected(ParseError{
                    .kind = ErrorKind::INVALID_VALUE,
                    .input_value = s,
                    .current_argument = std::optional{std::meta::display_string_of(^^M)},
                    .argv_index = index
                });
            return *parsed;
        } else {
            // TODO: can add more info to the msg?
            static_assert(false, "no conversion for this field type");
        }
    }

    consteval std::optional<Positional> positional_of(const std::meta::info m) {
        for (const std::meta::info a : std::meta::annotations_of(m)) {
            if (std::meta::dealias(std::meta::remove_cv(std::meta::type_of(a))) == std::meta::dealias(^^Positional)) {
                return std::meta::extract<Positional>(a);
            }
        }
        return std::nullopt;
    }

    consteval std::optional<Program> program_of(const std::meta::info r) {
        for (const std::meta::info a : std::meta::annotations_of(r)) {
            if (std::meta::dealias(std::meta::remove_cv(std::meta::type_of(a))) == std::meta::dealias(^^Program)) {
                return std::meta::extract<Program>(a);
            }
        }
        return std::nullopt;
    }

    consteval std::optional<Separator> separator_of(const std::meta::info m) {
        for (const std::meta::info a : std::meta::annotations_of(m)) {
            if (std::meta::dealias(std::meta::remove_cv(std::meta::type_of(a))) == std::meta::dealias(^^Separator)) {
                return std::meta::extract<Separator>(a);
            }
        }
        return std::nullopt;
    }

    consteval std::optional<std::meta::info> validator_of(const std::meta::info m) {
        for (const std::meta::info a : std::meta::annotations_of(m)) {
            auto type = std::meta::remove_cv(std::meta::type_of(a));
            if (is_specialization_of(type, ^^Validator)) {
                return a;
            }
        }
        return std::nullopt;
    }

    consteval std::optional<std::meta::info> parser_of(const std::meta::info m) {
        for (const std::meta::info a : std::meta::annotations_of(m)) {
            auto type = std::meta::remove_cv(std::meta::type_of(a));
            if (is_specialization_of(type, ^^Parser)) {
                return a;
            }
        }
        return std::nullopt;
    }

    consteval std::optional<std::meta::info> min_of(const std::meta::info m) {
        for (const std::meta::info a : std::meta::annotations_of(m)) {
            auto type = std::meta::remove_cv(std::meta::type_of(a));
            if (is_specialization_of(type, ^^Min)) {
                return a;
            }
        }
        return std::nullopt;
    }

    consteval std::optional<std::meta::info> max_of(const std::meta::info m) {
        for (const std::meta::info a : std::meta::annotations_of(m)) {
            auto type = std::meta::remove_cv(std::meta::type_of(a));
            if (is_specialization_of(type, ^^Max)) {
                return a;
            }
        }
        return std::nullopt;
    }

    consteval std::optional<std::meta::info> range_of(const std::meta::info m) {
        for (const std::meta::info a : std::meta::annotations_of(m)) {
            auto type = std::meta::remove_cv(std::meta::type_of(a));
            if (is_specialization_of(type, ^^Range)) {
                return a;
            }
        }
        return std::nullopt;
    }

    consteval std::optional<std::meta::info> choices_of(const std::meta::info m) {
        for (const std::meta::info a : std::meta::annotations_of(m)) {
            auto type = std::meta::remove_cv(std::meta::type_of(a));
            if (is_specialization_of(type, ^^Choices)) {
                return a;
            }
        }
        return std::nullopt;
    }

    consteval std::optional<detail::Subcommand_> subcommand_of(const std::meta::info m) {
        for (const std::meta::info a : std::meta::annotations_of(m)) {
            if (std::meta::dealias(std::meta::remove_cv(std::meta::type_of(a))) == std::meta::dealias(^^detail::Subcommand_)) {
                return std::meta::extract<detail::Subcommand_>(a);
            }
        }
        return std::nullopt;
    }

    template<std::meta::info M>
    consteval auto choices_list() {
        constexpr auto annotation = choices_of(M);
        using C = [:std::meta::remove_cv(std::meta::type_of(*annotation)):];
        constexpr auto choices = std::meta::extract<C>(*annotation);
        return std::span{choices.values_, choices.count_};
    }

    template<std::meta::info M, typename T>
    bool matches_choice(const T &value, const bool allow_case_insensitivity = false) {
        static constexpr auto choices = choices_list<M>();
        for (const auto &choice : choices) {
            if constexpr (std::convertible_to<T, std::string_view>) {
                if (std::string_view{value} == std::string_view{choice} || (
                        allow_case_insensitivity && iequals(value, choice)))
                    return true;
            } else {
                if (value == choice) return true;
            }
        }
        return false;
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
    consteval bool is_named_option(const std::meta::info m) {
        return !is_subcommand(m) && (opt_of(m).has_value() || positional_of(m).has_value() || is_bare<T>());
    }

    template<typename T>
    consteval std::optional<size_t> positional_index_of(const std::meta::info m) {
        if (is_subcommand(m)) {
            return std::nullopt;
        }
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
    bool option_takes_value(std::string_view name, bool is_short, const Settings &settings) {
        static constexpr auto members = std::define_static_array(
            std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        bool takes{false};
        template for (constexpr auto m : members) {
            using M = [:std::meta::type_of(m):];
            constexpr auto opt = opt_of(m);
            constexpr bool named = is_named_option<T>(m);
            if (named && does_match_name<m>(name, opt, settings, is_short)) takes = !is_flag_type(^^M);
        }
        return takes;
    }

    template<typename T>
    bool option_is_count(std::string_view name, bool is_short, const Settings &settings) {
        if (!settings.allow_repeated_counts) return false;
        static constexpr auto members = std::define_static_array(
            std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        template for (constexpr auto m : members) {
            if constexpr (is_named_option<T>(m) && is_count_type(std::meta::type_of(m))) {
                if (does_match_name<m>(name, opt_of(m), settings, is_short)) return true;
            }
        }
        return false;
    }

    consteval bool is_required(const std::meta::info m) {
        for (const auto a : std::meta::annotations_of(m)) {
            if (std::meta::dealias(std::meta::remove_cv(std::meta::type_of(a))) == std::meta::dealias(^^detail::Required_)) {
                return true;
            }
        }
        return false;
    }

    consteval bool is_positional(const std::meta::info m) {
        for (const auto a : std::meta::annotations_of(m)) {
            if (std::meta::dealias(std::meta::remove_cv(std::meta::type_of(a))) == std::meta::dealias(^^Positional)) {
                return true;
            }
        }
        return false;
    }

    template<typename F>
    void for_each_value(std::string_view s, const std::string_view separator, F &&f) {
        if (separator.empty()) {
            f(s);
            return;
        }
        auto pos{0uz};
        while ((pos = s.find(separator)) != std::string_view::npos) {
            f(s.substr(0, pos));
            s.remove_prefix(pos + separator.size());
        }
        f(s);
    }

    template<Numeric A, Numeric B>
    constexpr bool numeric_less(const A a, const B b) {
        constexpr bool both_integral = std::is_integral_v<A> && std::is_integral_v<B>;
        return both_integral ? std::cmp_less(a, b) : a < b;
    }

    template<std::meta::info Mem, typename M>
    bool validate(const M &value, const std::string_view input, const size_t index, std::vector<ParseError> &errors,
                  const Settings &settings) {
        constexpr auto validator_annotation = validator_of(Mem);
        constexpr auto choice_annotation = choices_of(Mem);
        constexpr auto min_annotation = min_of(Mem);
        constexpr auto max_annotation = max_of(Mem);
        constexpr auto range_annotation = range_of(Mem);

        if constexpr (validator_annotation.has_value()) {
            using V = [:std::meta::remove_cv(std::meta::type_of(*validator_annotation)):];
            constexpr auto validator = std::meta::extract<V>(*validator_annotation);

            static_assert(std::is_invocable_r_v<std::expected<void, std::string>, const V &, const M &>,
                          "Validator must accept the parsed value type");

            if (auto result = validator(value); !result) {
                errors.push_back({
                    .kind = ErrorKind::VALIDATION_FAILED,
                    .input_value = input,
                    .detail = result.error().empty()
                                  ? std::string{"validator returned false"}
                                  : std::move(result.error()),
                    .current_argument = std::meta::identifier_of(Mem),
                    .argv_index = index
                });
                return false;
            }
        } else if constexpr (choice_annotation.has_value()) {
            static constexpr auto choices = choices_list<Mem>();
            if (!matches_choice<Mem>(value, settings.allow_case_insensitivity)) {
                errors.push_back({
                    .kind = ErrorKind::INVALID_CHOICE,
                    .input_value = input,
                    .detail = std::format("{}", choices),
                    .current_argument = std::meta::identifier_of(Mem),
                    .argv_index = index
                });
                return false;
            }
        } else if constexpr (min_annotation.has_value()) {
            using A = [:std::meta::remove_cv(std::meta::type_of(*min_annotation)):];
            constexpr auto mn = std::meta::extract<A>(*min_annotation);

            if (numeric_less(value, mn.value)) {
                errors.push_back({
                    .kind = ErrorKind::OUT_OF_RANGE,
                    .input_value = input,
                    .detail = std::format("minimum is {}", mn.value),
                    .current_argument = std::meta::identifier_of(Mem),
                    .argv_index = index
                });
                return false;
            }
        } else if constexpr (max_annotation.has_value()) {
            using A = [:std::meta::remove_cv(std::meta::type_of(*max_annotation)):];
            constexpr auto mx = std::meta::extract<A>(*max_annotation);

            if (numeric_less(mx.value, value)) {
                errors.push_back({
                    .kind = ErrorKind::OUT_OF_RANGE,
                    .input_value = input,
                    .detail = std::format("maximum is {}", mx.value),
                    .current_argument = std::meta::identifier_of(Mem),
                    .argv_index = index
                });
                return false;
            }
        } else if constexpr (range_annotation.has_value()) {
            using A = [:std::meta::remove_cv(std::meta::type_of(*range_annotation)):];
            constexpr auto range = std::meta::extract<A>(*range_annotation);

            if (numeric_less(value, range.min) || numeric_less(range.max, value)) {
                errors.push_back({
                    .kind = ErrorKind::OUT_OF_RANGE,
                    .input_value = input,
                    .detail = std::format("allowed range is [{}, {}]", range.min, range.max),
                    .current_argument = std::meta::identifier_of(Mem),
                    .argv_index = index
                });
                return false;
            }
        }
        return true;
    }

    template<std::meta::info Mem, typename M>
    bool increment_count(M &field, const std::string_view name, const size_t index,
                         std::vector<ParseError> &errors, const Settings &settings) {
        if constexpr (is_optional(^^M)) {
            auto value = field.value_or(0);
            if (!increment_count<Mem>(value, name, index, errors, settings)) return false;
            field = value;
        } else {
            if (field == std::numeric_limits<M>::max()) {
                errors.push_back({
                    .kind = ErrorKind::OUT_OF_RANGE, .input_value = name,
                    .detail = "count would overflow", .current_argument = std::meta::identifier_of(Mem),
                    .argv_index = index
                });
                return false;
            }
            const M value = field + M{1};
            if (!validate<Mem>(value, name, index, errors, settings)) return false;
            field = value;
        }
        return true;
    }

    template<std::meta::info Mem, typename E>
    std::expected<E, ParseError> apply_custom_parser(const std::string_view s, const size_t index) {
        constexpr auto parser = *parser_of(Mem);
        using V = [:std::meta::remove_cv(std::meta::type_of(parser)):];
        constexpr auto parsing_function = std::meta::extract<V>(parser);

        static_assert(std::is_invocable_r_v<std::expected<E, std::string>, const V &, const std::string_view>,
                      "Parser must accept a string_view and return std::expected of the parsed value type and std::string");
        if (auto result = parsing_function(s); !result) {
            return std::unexpected(ParseError{
                .kind = ErrorKind::CUSTOM_PARSING_FAILED,
                .input_value = s,
                .detail = result.error().empty()
                              ? std::string{"parsed returned an error"}
                              : std::move(result.error()),
                .current_argument = std::meta::identifier_of(Mem),
                .argv_index = index
            });
        } else return *result;
    }

    template<std::meta::info Mem, typename M>
    void assign_from_string(M &field, const std::string_view s, const size_t index, size_t &count,
                            std::vector<ParseError> &errors, const Settings &settings) {
        constexpr auto annotated = separator_of(Mem);
        constexpr bool has_custom_parser = parser_of(Mem).has_value();
        const std::string_view separator = annotated.has_value()
                                               ? std::string_view{annotated->value}
                                               : settings.value_separator;
        if constexpr (is_vector(std::meta::remove_cv(^^M))) {
            using E = [:value_type_of(^^M):];
            if (count == 0) field.clear();
            for_each_value(s, separator, [&](auto piece) {
                if constexpr (has_custom_parser) {
                    if (auto result = apply_custom_parser<Mem, E>(piece, index)) {
                        if (validate<Mem>(*result, piece, index, errors, settings)) {
                            field.push_back(*result);
                        }
                    } else errors.push_back(result.error());
                } else if (auto result = from_string<E>(piece, index, settings)) {
                    if (validate<Mem>(*result, piece, index, errors, settings)) {
                        field.push_back(*result);
                    }
                } else errors.push_back(result.error());
                count++;
            });
        } else if constexpr (is_array(std::meta::remove_cv(^^M))) {
            using E = [:value_type_of(^^M):];
            for_each_value(s, separator, [&](auto piece) {
                if (count >= std::tuple_size_v<M>) {
                    errors.push_back(ParseError{
                        .kind = ErrorKind::DUPLICATE_OPTION,
                        .input_value = piece,
                        .current_argument = std::meta::identifier_of(Mem),
                        .argv_index = index
                    });
                } else if constexpr (has_custom_parser) {
                    if (auto result = apply_custom_parser<Mem, E>(piece, index)) {
                        if (validate<Mem>(*result, piece, index, errors, settings)) {
                            field[count] = *result;
                        }
                    } else errors.push_back(result.error());
                } else if (auto result = from_string<E>(piece, index, settings)) {
                    if (validate<Mem>(*result, piece, index, errors, settings)) {
                        field[count] = *result;
                    }
                } else errors.push_back(result.error());
                count++;
            });
        } else if constexpr (has_custom_parser) {
            if (auto result = apply_custom_parser<Mem, M>(s, index); !result) {
                errors.push_back(result.error());
            } else field = *result;
            count++;
        } else {
            if (auto result = from_string<M>(s, index, settings)) {
                if (validate<Mem>(*result, s, index, errors, settings)) {
                    field = *result;
                }
            } else errors.push_back(result.error());
            count++;
        }
    }
}
