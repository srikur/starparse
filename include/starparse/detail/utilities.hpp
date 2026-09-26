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

// TODO: split utilities into multiple files?
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
    bool matches_alias(const std::string_view name, const bool allow_case_insensitivity = false) {
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

    consteval std::string_view name_of(const std::meta::info entity) {
        if (std::meta::is_nonstatic_data_member(entity)) {
            for (const auto annotation : std::meta::annotations_of(entity)) {
                if (std::meta::dealias(std::meta::remove_cv(std::meta::type_of(annotation))) == ^^Name) {
                    return std::meta::extract<Name>(annotation).name_;
                }
            }
        }
        return std::meta::identifier_of(entity);
    }

    template<std::meta::info M>
    inline constexpr std::string_view snake_name_v = name_of(M);

    template<std::meta::info M>
    inline constexpr std::string_view kebab_name_v = [] {
        std::string s(name_of(M));
        std::ranges::replace(s, '_', '-');
        return std::string_view(std::define_static_string(s), s.size());
    }();

    template<std::meta::info M>
    inline constexpr std::string_view snake_negated_name_v = [] {
        std::string s(name_of(M));
        s.reserve(s.size() + 3);
        s.append("no-");
        return std::string_view(std::define_static_string(s), s.size());
    }();

    template<std::meta::info M>
    inline constexpr std::string_view kebab_negated_name_v = [] {
        std::string s(name_of(M));
        s.reserve(s.size() + 3);
        s.append("no_");
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
    constexpr bool matches_non_negated_name(const std::string_view name,
                                            const std::optional<Opt> &opt,
                                            const Settings &settings,
                                            const bool allow_short = true) {
        if (allow_short && name.size() == 1 && opt.has_value() && name[0] == opt->short_name)
            return true;
        if (check_snake_case<M>(name, settings) || check_kebab_case<M>(name, settings))
            return true;
        if (settings.allow_aliases && Utilities::matches_alias<M>(name, settings.allow_case_insensitivity))
            return true;
        return false;
    }

    template<std::meta::info M>
    constexpr bool does_match_name(const std::string_view name,
                                   const std::optional<Opt> &opt,
                                   const Settings &settings,
                                   const bool allow_short = true) {
        if (matches_non_negated_name<M>(name, opt, settings, allow_short)) return true;
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
        return std::unexpected(ParseError{.kind = ErrorKind::INVALID_VALUE, .input_value = std::string{s}});
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
                    .input_value = std::string{s},
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
                    .input_value = std::string{s},
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

    consteval std::optional<File> file_of(const std::meta::info m) {
        for (const std::meta::info a : std::meta::annotations_of(m)) {
            if (std::meta::dealias(std::meta::remove_cv(std::meta::type_of(a))) == std::meta::dealias(^^File)) {
                return std::meta::extract<File>(a);
            }
        }
        return std::nullopt;
    }

    consteval std::optional<Env> env_of(const std::meta::info m) {
        for (const std::meta::info a : std::meta::annotations_of(m)) {
            if (std::meta::dealias(std::meta::remove_cv(std::meta::type_of(a))) == std::meta::dealias(^^Env)) {
                return std::meta::extract<Env>(a);
            }
        }
        return std::nullopt;
    }

    template<std::meta::info M>
    inline constexpr std::string_view env_name_v = [] {
        constexpr auto env = env_of(M);
        static_assert(env.has_value(), "member requires an Env annotation");
        std::string s{env->name};
        for (auto &c : s) {
            // note: apparently std::toupper is not constexpr
            if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
        }
        return std::string_view{std::define_static_string(s), s.size()};
    }();

    template<std::meta::info M>
    consteval auto choices_list() {
        constexpr auto annotation = choices_of(M);
        using C = [:std::meta::remove_cv(std::meta::type_of(*annotation)):];
        constexpr auto choices = std::meta::extract<C>(*annotation);
        return std::span{choices.values_, choices.count_};
    }

    template<std::meta::info M, typename T>
    bool matches_choice(const T &value, const bool allow_case_insensitivity = false) {
        if constexpr (!choices_of(M).has_value()) {
            return false;
        } else {
            static constexpr auto choices = choices_list<M>();
            for (const auto &choice : choices) {
                if constexpr (std::convertible_to<T, std::string_view> &&
                              std::convertible_to<decltype(choice), std::string_view>) {
                    if (std::string_view{value} == std::string_view{choice} || (
                            allow_case_insensitivity && iequals(value, choice)))
                        return true;
                } else if constexpr (requires { { value == choice } -> std::convertible_to<bool>; }) {
                    if (value == choice) return true;
                }
            }
            return false;
        }
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
        for (const std::meta::info member : std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current())) {
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

    [[nodiscard]] inline std::string to_uppercase(const std::string_view sv) {
        std::string result{sv};
        std::ranges::transform(result, result.begin(), [](unsigned char c) {
            return std::toupper(c);
        });
        return result;
    }

    constexpr char ascii_lower(const char c) {
        return c >= 'A' && c <= 'Z' ? static_cast<char>(c + ('a' - 'A')) : c;
    }

    constexpr bool same_name(const std::string_view a, const std::string_view b, const bool case_sensitive = false) {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i) {
            if (case_sensitive ? a[i] != b[i] : ascii_lower(a[i]) != ascii_lower(b[i])) return false;
        }
        return true;
    }

    template<std::meta::info M>
    bool matches_short_name(const std::string_view &value, const bool allow_case_insensitivity = false) {
        constexpr std::optional<Opt> opt = opt_of(M);
        if (opt.has_value() && value.size() == 1) {
            return allow_case_insensitivity ? ascii_lower(value[0]) == ascii_lower(opt->short_name) : value[0] == opt->short_name;
        }
        return false;
    }

    template<typename T>
    bool short_name_exists(const std::string_view name, const bool allow_case_insensitivity = false) {
        // check aliases, opt, and choices
        static constexpr auto members = std::define_static_array(
            std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        template for (constexpr auto member : members) {
            if constexpr (is_named_option<T>(member)) {
                constexpr auto canonical = name_of(member);
                if (canonical.size() == 1 &&
                    (name == canonical || (allow_case_insensitivity && iequals(name, canonical))))
                    return true;
            }
            if (matches_alias<member>(name, allow_case_insensitivity)
                || matches_choice<member>(name, allow_case_insensitivity)
                || matches_short_name<member>(name, allow_case_insensitivity)) {
                return true;
            }
        }
        return false;
    }

    template<typename T>
    std::vector<std::string_view> viable_candidate_names(const Settings &settings) {
        std::vector<std::string_view> names;
        static constexpr auto members = std::define_static_array(
            std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        template for (constexpr auto m : members) {
            if (is_named_option<T>(m)) {
                // TODO: support case insensitivity here?
                names.push_back(snake_name_v<m>);
                if (settings.allow_kebab_casing) {
                    names.push_back(kebab_name_v<m>);
                }
                if (settings.allow_aliases) {
                    for (const auto name : alias_name_list(m)) {
                        names.push_back(name);
                    }
                }
                if (settings.autogenerate_negations && is_flag_type(std::meta::type_of(m))) {
                    names.push_back(snake_negated_name_v<m>);
                    if (settings.allow_kebab_casing) names.push_back(kebab_negated_name_v<m>);
                }
                names.push_back("--help");
                names.push_back("--version");
            }
        }
        return names;
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
                    .input_value = std::string{input},
                    .detail = result.error().empty()
                                  ? std::string{"validator returned false"}
                                  : std::move(result.error()),
                    .current_argument = name_of(Mem),
                    .argv_index = index
                });
                return false;
            }
        } else if constexpr (choice_annotation.has_value()) {
            static constexpr auto choices = choices_list<Mem>();
            if (!matches_choice<Mem>(value, settings.allow_case_insensitivity)) {
                errors.push_back({
                    .kind = ErrorKind::INVALID_CHOICE,
                    .input_value = std::string{input},
                    .detail = std::format("{}", choices),
                    .current_argument = name_of(Mem),
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
                    .input_value = std::string{input},
                    .detail = std::format("minimum is {}", mn.value),
                    .current_argument = name_of(Mem),
                    .argv_index = index
                });
                return false;
            }
        } else if constexpr (max_annotation.has_value()) {
            using A = [:std::meta::remove_cv(std::meta::type_of(*max_annotation)):];
            constexpr auto mx = std::meta::extract<A>(*max_annotation);

            if (numeric_less(mx.value, value)) {
                errors.push_back(ParseError{
                    .kind = ErrorKind::OUT_OF_RANGE,
                    .input_value = std::string{input},
                    .detail = std::format("maximum is {}", mx.value),
                    .current_argument = name_of(Mem),
                    .argv_index = index
                });
                return false;
            }
        } else if constexpr (range_annotation.has_value()) {
            using A = [:std::meta::remove_cv(std::meta::type_of(*range_annotation)):];
            constexpr auto range = std::meta::extract<A>(*range_annotation);

            if (numeric_less(value, range.min) || numeric_less(range.max, value)) {
                errors.push_back(ParseError{
                    .kind = ErrorKind::OUT_OF_RANGE,
                    .input_value = std::string{input},
                    .detail = std::format("allowed range is [{}, {}]", range.min, range.max),
                    .current_argument = name_of(Mem),
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
                errors.push_back(ParseError{
                    .kind = ErrorKind::OUT_OF_RANGE, .input_value = std::string{name},
                    .detail = "count would overflow", .current_argument = name_of(Mem),
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
                .input_value = std::string{s},
                .detail = result.error().empty()
                              ? std::string{"parsed returned an error"}
                              : std::move(result.error()),
                .current_argument = name_of(Mem),
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
                        .input_value = std::string{piece},
                        .current_argument = name_of(Mem),
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

    [[nodiscard]] constexpr size_t edit_distance(const std::string_view a, const std::string_view b) {
        const auto m = a.size();
        const auto n = b.size();
        if (m == 0) return n;
        if (n == 0) return m;
        if (a == b) return 0;

        if (constexpr auto max_size = std::numeric_limits<size_t>::max(); m == max_size || n == max_size) {
            return 0;
        }

        const auto rows = m + 1;
        const auto cols = n + 1;
        std::vector<size_t> matrix;
        if (rows > matrix.max_size() / cols) return 0;
        matrix.resize(rows * cols);

        const auto at = [&matrix, cols](const size_t i, const size_t j) -> size_t & {
            return matrix[i * cols + j];
        };

        for (auto i{0uz}; i <= m; ++i) at(i, 0) = i;
        for (auto j{0uz}; j <= n; ++j) at(0, j) = j;

        constexpr size_t alphabet_size =
                static_cast<size_t>(std::numeric_limits<unsigned char>::max()) + 1;
        std::array<size_t, alphabet_size> last_row{};

        // TODO: modification — need to treat _ and - as equal when kebab casing is enabled
        for (auto i{1uz}; i <= m; ++i) {
            auto last_match_col{0uz};
            const auto ac = static_cast<unsigned char>(a[i - 1]);

            for (auto j{1uz}; j <= n; ++j) {
                const auto bc = static_cast<unsigned char>(b[j - 1]);
                const auto previous_row = last_row[bc];
                const auto previous_col = last_match_col;
                const auto cost = ac == bc ? 0uz : 1uz;

                if (cost == 0)
                    last_match_col = j;

                auto best = std::min({
                    at(i - 1, j) + 1, // deletion
                    at(i, j - 1) + 1, // insertion
                    at(i - 1, j - 1) + cost // substitution
                });

                if (previous_row != 0 && previous_col != 0) {
                    const auto transposition = at(previous_row - 1, previous_col - 1)
                                               + (i - previous_row - 1)
                                               + 1
                                               + (j - previous_col - 1);
                    best = std::min(best, transposition);
                }
                at(i, j) = best;
            }
            last_row[ac] = i;
        }
        return at(m, n);
    }
}
