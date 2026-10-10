#pragma once

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <concepts>
#include <expected>
#include <limits>
#include <meta>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <starparse/detail/annotations.hpp>
#include <starparse/detail/errors.hpp>
#include <starparse/detail/settings.hpp>

// TODO: split utilities into multiple files?
namespace StarParse::detail::Utilities {
    consteval bool is_specialization_of(std::meta::info type, const std::meta::info templ) {
        type = std::meta::dealias(type);
        return std::meta::has_template_arguments(type) && std::meta::template_of(type) == templ;
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

    template<typename T>
    consteval std::optional<T> extraction_of(const std::meta::info m) {
        for (const std::meta::info a : std::meta::annotations_of(m)) {
            if (std::meta::dealias(std::meta::remove_cv(std::meta::type_of(a))) == std::meta::dealias(^^T)) {
                return std::meta::extract<T>(a);
            }
        }
        return std::nullopt;
    }

    template<std::meta::info T>
    consteval std::optional<std::meta::info> specialization_of(const std::meta::info m) {
        for (const std::meta::info a : std::meta::annotations_of(m)) {
            auto type = std::meta::remove_cv(std::meta::type_of(a));
            if (is_specialization_of(type, T)) {
                return a;
            }
        }
        return std::nullopt;
    }

    template<typename Annotation>
    consteval bool has_annotation(const std::meta::info m) {
        for (const auto a : std::meta::annotations_of(m)) {
            if (std::meta::dealias(std::meta::remove_cv(std::meta::type_of(a))) == std::meta::dealias(^^Annotation)) {
                return true;
            }
        }
        return false;
    }

    template<typename T>
    consteval bool is_bare() {
        for (const std::meta::info m : std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current())) {
            if (extraction_of<Opt>(m).has_value() || extraction_of<Positional>(m).has_value()) {
                return false;
            }
        }
        return true;
    }

    template<typename T>
    consteval bool is_named_option(const std::meta::info m) {
        return !has_annotation<detail::Subcommand_>(m) && (extraction_of<Opt>(m).has_value()
                                                           || extraction_of<Positional>(m).has_value()
                                                           || is_bare<T>());
    }

    consteval bool is_hidden(const std::meta::info m) {
        for (const auto a : std::meta::annotations_of(m)) {
            if (std::meta::dealias(std::meta::remove_cv(std::meta::type_of(a))) == std::meta::dealias(^^detail::Hidden_)) {
                return true;
            }
        }
        return false;
    }

    constexpr char ascii_lower(const char c) {
        return c >= 'A' && c <= 'Z' ? static_cast<char>(c + ('a' - 'A')) : c;
    }

    template<std::meta::info M>
    bool matches_short_name(const std::string_view value, const bool allow_case_insensitivity = false) {
        constexpr std::optional<Opt> opt = extraction_of<Opt>(M);
        if (opt.has_value() && value.size() == 1) {
            return allow_case_insensitivity ? ascii_lower(value[0]) == ascii_lower(opt->short_name) : value[0] == opt->short_name;
        }
        return false;
    }

    inline bool iequals(const std::string_view a, const std::string_view b) {
        if (a.length() != b.length()) {
            return false;
        }
        return std::equal(a.begin(), a.end(), b.begin(),
                          [](const unsigned char ac, const unsigned char bc) {
                              return std::tolower(ac) == std::tolower(bc);
                          });
    }

    consteval std::vector<std::string_view> alias_name_list(const std::meta::info m) {
        std::vector<std::string_view> names{};
        for (const std::meta::info a : std::meta::annotations_of(m)) {
            if (std::meta::dealias(std::meta::remove_cv(std::meta::type_of(a))) != std::meta::dealias(^^Alias)) {
                continue;
            }
            const auto alias = std::meta::extract<Alias>(a);
            for (auto i{0uz}; i < alias.count_; i++) {
                names.push_back(std::string_view{alias.names_[i]});
            }
        }
        return names;
    }

    template<std::meta::info M>
    constexpr std::span<const std::string_view> alias_names() {
        // string_view is not structural, so it cannot be used with define_static_array
        static constexpr auto aliases = [] {
            std::array<std::string_view, alias_name_list(M).size()> names{};
            std::ranges::copy(alias_name_list(M), names.begin());
            return names;
        }();
        return aliases;
    }

    template<std::meta::info M>
    bool matches_alias(const std::string_view name, const bool allow_case_insensitivity = false) {
        for (const auto &alias : alias_names<M>()) {
            if (name == alias || (allow_case_insensitivity && iequals(name, alias)))
                return true;
        }
        return false;
    }

    template<std::meta::info M>
    consteval auto choices_list() {
        constexpr auto annotation = specialization_of<^^Choices>(M);
        using C = [:std::meta::remove_cv(std::meta::type_of(*annotation)):];
        constexpr auto choices = std::meta::extract<C>(*annotation);
        return std::span{choices.values_, choices.count_};
    }

    template<std::meta::info M, std::meta::info T>
    consteval auto get_annotation_list_values() {
        constexpr auto annotation = specialization_of<T>(M);
        using C = [:std::meta::remove_cv(std::meta::type_of(*annotation)):];
        constexpr auto values = std::meta::extract<C>(*annotation);
        return std::span{values.values_, values.count_};
    }

    template<std::meta::info M, typename T>
    bool matches_choice(const T &value, const bool allow_case_insensitivity = false) {
        if constexpr (!specialization_of<^^Choices>(M).has_value()) {
            return false;
        } else {
            static constexpr auto choices = choices_list<M>();
            for (const auto &choice : choices) {
                if constexpr (std::convertible_to<T, std::string_view> && std::convertible_to<decltype(choice), std::string_view>) {
                    if (std::string_view{value} == std::string_view{choice} || (allow_case_insensitivity && iequals(value, choice)))
                        return true;
                } else if constexpr (requires
                {
                    { value == choice } -> std::convertible_to<bool>;
                }) {
                    if (value == choice)
                        return true;
                }
            }
            return false;
        }
    }

    template<typename T>
    bool short_name_exists(const std::string_view name, const bool allow_case_insensitivity = false) {
        // check aliases, opt, and choices
        static constexpr auto members = std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        template for (constexpr auto member : members) {
            if constexpr (is_named_option<T>(member)) {
                constexpr auto canonical = name_of(member);
                if (canonical.size() == 1 && (name == canonical || (allow_case_insensitivity && iequals(name, canonical))))
                    return true;
            }
            if (matches_alias<member>(name, allow_case_insensitivity) || matches_choice<member>(name, allow_case_insensitivity) ||
                matches_short_name<member>(name, allow_case_insensitivity)) {
                return true;
            }
        }
        return false;
    }

    consteval bool is_optional(std::meta::info r) {
        r = std::meta::dealias(r);
        return std::meta::has_template_arguments(r) && std::meta::template_of(r) == ^^std::optional;
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
        if (is_optional(r))
            r = std::meta::dealias(value_type_of(r));
        return std::meta::is_integral_type(r) && r != (^^bool) && !std::meta::extract<bool>(std::meta::substitute(^^is_char_v, {
                       r
                   }));
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
        s.insert(0, "no-");
        return std::string_view(std::define_static_string(s), s.size());
    }();

    template<std::meta::info M>
    inline constexpr std::string_view kebab_negated_name_v = [] {
        std::string s(name_of(M));
        s.reserve(s.size() + 3);
        s.insert(0, "no_");
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
    constexpr bool matches_non_negated_name(const std::string_view name, const Settings &settings, const bool allow_short = true) {
        if (allow_short && matches_short_name<M>(name, settings.allow_case_insensitivity))
            return true;
        if (check_snake_case<M>(name, settings) || check_kebab_case<M>(name, settings))
            return true;
        if (settings.allow_aliases && Utilities::matches_alias<M>(name, settings.allow_case_insensitivity))
            return true;
        return false;
    }

    template<std::meta::info M>
    constexpr bool does_match_name(const std::string_view name, const Settings &settings, const bool allow_short = true) {
        if (matches_non_negated_name<M>(name, settings, allow_short))
            return true;
        // checking name size >= 3 prevents GCC complaining about removing a prefix of size 3
        if (settings.autogenerate_negations && name.size() >= 3 && name.starts_with("no-") && is_flag_type(std::meta::type_of(M))) {
            std::string_view negated{name};
            negated.remove_prefix(3);
            return check_snake_case<M>(negated, settings) || check_kebab_case<M>(negated, settings);
        }
        return false;
    }

    consteval bool is_vector(const std::meta::info r) {
        return is_specialization_of(r, ^^std::vector);
    }

    consteval bool is_array(const std::meta::info r) {
        return is_specialization_of(r, ^^std::array);
    }

    consteval bool is_container(const std::meta::info m) {
        return is_vector(m) || is_array(m);
    }

    template<std::meta::info M>
    constexpr std::expected<bool, ParseError> bool_from_string(const std::string_view s) {
        using namespace std::literals;
        constexpr std::array true_values{"yes"sv, "1"sv, "on"sv, "true"sv, "t"sv};
        constexpr std::array false_values{"no"sv, "0"sv, "off"sv, "false"sv, "f"sv};
        if (std::ranges::any_of(true_values, [&](auto value) {
            return iequals(s, value);
        }))
            return true;
        if (std::ranges::any_of(false_values, [&](auto value) {
            return iequals(s, value);
        }))
            return false;
        return std::unexpected(ParseError{
            .kind = ErrorKind::INVALID_VALUE,
            .input_value = std::string{s},
            .current_argument = name_of(M),
        });
    }

    template<typename T>
    consteval size_t member_index_of(const std::meta::info m) {
        size_t index{0};
        for (const std::meta::info member : std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current())) {
            if (member == m) {
                return index;
            }
            ++index;
        }
        throw std::invalid_argument("member not found");
    }

    template<std::meta::info Mem, typename M>
    std::expected<M, ParseError> from_string(std::string_view s, const size_t index, const Settings &settings) {
        if constexpr (is_optional(^^M)) {
            using T = [:value_type_of(^^M):];
            if (auto result = from_string<Mem, T>(s, index, settings))
                return M{*result};
            else
                return std::unexpected(result.error());
        } else if constexpr (std::same_as<M, bool>) {
            if (auto result = bool_from_string<Mem>(s))
                return M{*result};
            else
                return std::unexpected(result.error());
        } else if constexpr (std::constructible_from<M, std::string_view>) {
            return M{s};
        } else if constexpr (is_char_v<M>) {
            return s.empty() ? M{0} : M{s[0]};
        } else if constexpr (std::is_arithmetic_v<M>) {
            M v{};
            auto [pointer, error_code] = std::from_chars(s.data(), s.data() + s.size(), v);
            if (error_code != std::errc{} || pointer != s.data() + s.size()) {
                return std::unexpected(ParseError{
                    .kind = ErrorKind::INVALID_VALUE, .input_value = std::string{s}, .current_argument = name_of(Mem), .argv_index = index
                });
            }
            return v;
        } else if constexpr (std::is_enum_v<M>) {
            std::optional<M> parsed;
            template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^M))) {
                if (!parsed.has_value() && does_match_name<e>(s, settings)) {
                    parsed = [:e:];
                }
            }
            if (!parsed)
                return std::unexpected(ParseError{
                    .kind = ErrorKind::INVALID_VALUE, .input_value = std::string{s}, .current_argument = name_of(Mem), .argv_index = index
                });
            return *parsed;
        } else {
            // TODO: can add more info to the msg?
            static_assert(false, "no conversion for this field type");
        }
    }

    template<std::meta::info M>
    inline constexpr std::string_view env_name_v = [] {
        constexpr auto env = extraction_of<Env>(M);
        static_assert(env.has_value(), "member requires an Env annotation");
        std::string s{env->name};
        for (auto &c : s) {
            // note: apparently std::toupper is not constexpr
            if (c >= 'a' && c <= 'z')
                c = static_cast<char>(c - 'a' + 'A');
        }
        return std::string_view{std::define_static_string(s), s.size()};
    }();

    template<typename T>
    consteval std::optional<size_t> positional_index_of(const std::meta::info m) {
        if (has_annotation<detail::Subcommand_>(m)) {
            return std::nullopt;
        }
        if (auto pos = extraction_of<Positional>(m)) {
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
        static constexpr auto members = std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        bool takes{false};
        template for (constexpr auto m : members) {
            using M = [:std::meta::type_of(m):];
            constexpr bool named = is_named_option<T>(m);
            if (named && does_match_name<m>(name, settings, is_short))
                takes = !is_flag_type(^^M);
        }
        return takes;
    }

    template<typename T>
    bool option_is_count(std::string_view name, bool is_short, const Settings &settings) {
        if (!settings.allow_repeated_counts)
            return false;
        static constexpr auto members = std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        template for (constexpr auto m : members) {
            if constexpr (is_named_option<T>(m) && is_count_type(std::meta::type_of(m))) {
                if (does_match_name<m>(name, settings, is_short))
                    return true;
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

    constexpr bool same_name(const std::string_view a, const std::string_view b, const bool case_sensitive = false) {
        if (a.size() != b.size())
            return false;
        for (size_t i = 0; i < a.size(); ++i) {
            if (case_sensitive ? a[i] != b[i] : ascii_lower(a[i]) != ascii_lower(b[i]))
                return false;
        }
        return true;
    }

    template<typename T>
    std::vector<std::string_view> viable_candidate_names(const Settings &settings) {
        std::vector<std::string_view> names;
        static constexpr auto members = std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        template for (constexpr auto m : members) {
            if (is_named_option<T>(m)) {
                // TODO: support case insensitivity here?
                names.push_back(snake_name_v<m>);
                if (settings.allow_kebab_casing) {
                    names.push_back(kebab_name_v<m>);
                }
                if (settings.allow_aliases) {
                    for (const auto &alias : alias_names<m>()) {
                        names.push_back(alias);
                    }
                }
                if (settings.autogenerate_negations && is_flag_type(std::meta::type_of(m))) {
                    names.push_back(snake_negated_name_v<m>);
                    if (settings.allow_kebab_casing)
                        names.push_back(kebab_negated_name_v<m>);
                }
                names.push_back("help");
                names.push_back("version");
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
    bool validate(const M &value, const std::string_view input, const size_t index, std::vector<ParseError> &errors, const Settings &settings) {
        constexpr auto validator_annotation = specialization_of<^^Validator>(Mem);
        constexpr auto choice_annotation = specialization_of<^^Choices>(Mem);
        constexpr auto min_annotation = specialization_of<^^Min>(Mem);
        constexpr auto max_annotation = specialization_of<^^Max>(Mem);
        constexpr auto range_annotation = specialization_of<^^Range>(Mem);

        if constexpr (validator_annotation.has_value()) {
            using V = [:std::meta::remove_cv(std::meta::type_of(*validator_annotation)):];
            constexpr auto validator = std::meta::extract<V>(*validator_annotation);

            static_assert(std::is_invocable_r_v<std::expected<void, std::string>, const V &, const M &>,
                          "Validator must accept the parsed value type");

            if (auto result = validator(value); !result) {
                errors.push_back({
                    .kind = ErrorKind::VALIDATION_FAILED,
                    .input_value = std::string{input},
                    .detail = result.error().empty() ? std::string{"validator returned false"} : std::move(result.error()),
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
    bool increment_count(M &field, const std::string_view name, const size_t index, std::vector<ParseError> &errors, const Settings &settings) {
        if constexpr (is_optional(^^M)) {
            auto value = field.value_or(0);
            if (!increment_count<Mem>(value, name, index, errors, settings))
                return false;
            field = value;
        } else {
            if (field == std::numeric_limits<M>::max()) {
                errors.push_back(ParseError{
                    .kind = ErrorKind::OUT_OF_RANGE,
                    .input_value = std::string{name},
                    .detail = "count would overflow",
                    .current_argument = name_of(Mem),
                    .argv_index = index
                });
                return false;
            }
            const M value = field + M{1};
            if (!validate<Mem>(value, name, index, errors, settings))
                return false;
            field = value;
        }
        return true;
    }

    template<std::meta::info Mem, typename E>
    std::expected<E, ParseError> apply_custom_parser(const std::string_view s, const size_t index) {
        constexpr auto parser = *specialization_of<^^Parser>(Mem);
        using V = [:std::meta::remove_cv(std::meta::type_of(parser)):];
        constexpr auto parsing_function = std::meta::extract<V>(parser);

        static_assert(std::is_invocable_r_v<std::expected<E, std::string>, const V &, const std::string_view>,
                      "Parser must accept a string_view and return std::expected of the parsed value type and std::string");
        if (auto result = parsing_function(s); !result) {
            return std::unexpected(ParseError{
                .kind = ErrorKind::CUSTOM_PARSING_FAILED,
                .input_value = std::string{s},
                .detail = result.error().empty() ? std::string{"parsed returned an error"} : std::move(result.error()),
                .current_argument = name_of(Mem),
                .argv_index = index
            });
        } else
            return *result;
    }

    template<std::meta::info Mem, typename M>
    void assign_from_string(M &field, const std::string_view s, const size_t index, size_t &count, std::vector<ParseError> &errors,
                            const Settings &settings) {
        constexpr auto annotated = extraction_of<Separator>(Mem);
        constexpr bool has_custom_parser = specialization_of<^^Parser>(Mem).has_value();
        // lifetime of the char should be okay I think
        const unsigned char raw_sep = annotated.has_value() ? annotated->value : settings.value_separator;
        const std::string_view separator = std::string_view{reinterpret_cast<const char *>(&raw_sep), 1};
        if constexpr (is_vector(std::meta::remove_cv(^^M))) {
            using E = [:value_type_of(^^M):];
            if (count == 0)
                field.clear();
            for_each_value(s, separator, [&](auto piece) {
                if constexpr (has_custom_parser) {
                    if (auto result = apply_custom_parser<Mem, E>(piece, index)) {
                        if (validate<Mem>(*result, piece, index, errors, settings)) {
                            field.push_back(*result);
                        }
                    } else
                        errors.push_back(result.error());
                } else if (auto result = from_string<Mem, E>(piece, index, settings)) {
                    if (validate<Mem>(*result, piece, index, errors, settings)) {
                        field.push_back(*result);
                    }
                } else
                    errors.push_back(result.error());
                count++;
            });
        } else if constexpr (is_array(std::meta::remove_cv(^^M))) {
            using E = [:value_type_of(^^M):];
            for_each_value(s, separator, [&](auto piece) {
                if (count >= std::tuple_size_v<M>) {
                    errors.push_back(ParseError{
                        .kind = ErrorKind::DUPLICATE_VALUE,
                        .input_value = std::string{piece},
                        .current_argument = name_of(Mem),
                        .argv_index = index
                    });
                } else if constexpr (has_custom_parser) {
                    if (auto result = apply_custom_parser<Mem, E>(piece, index)) {
                        if (validate<Mem>(*result, piece, index, errors, settings)) {
                            field[count] = *result;
                        }
                    } else
                        errors.push_back(result.error());
                } else if (auto result = from_string<Mem, E>(piece, index, settings)) {
                    if (validate<Mem>(*result, piece, index, errors, settings)) {
                        field[count] = *result;
                    }
                } else
                    errors.push_back(result.error());
                count++;
            });
        } else if constexpr (has_custom_parser) {
            if (auto result = apply_custom_parser<Mem, M>(s, index)) {
                if (validate<Mem>(*result, s, index, errors, settings)) {
                    field = *result;
                }
            } else
                errors.push_back(result.error());
            count++;
        } else {
            if (auto result = from_string<Mem, M>(s, index, settings)) {
                if (validate<Mem>(*result, s, index, errors, settings)) {
                    field = *result;
                }
            } else
                errors.push_back(result.error());
            count++;
        }
    }

    [[nodiscard]] constexpr size_t edit_distance(const std::string_view a, const std::string_view b, const Settings &settings) {
        const auto m = a.size();
        const auto n = b.size();
        if (m == 0)
            return n;
        if (n == 0)
            return m;
        if (a == b)
            return 0;

        if (constexpr auto max_size = std::numeric_limits<size_t>::max(); m == max_size || n == max_size) {
            return 0;
        }

        const auto rows = m + 1;
        const auto cols = n + 1;
        std::vector<size_t> matrix;
        if (rows > matrix.max_size() / cols)
            return 0;
        matrix.resize(rows * cols);

        const auto at = [&matrix, cols](const size_t i, const size_t j) -> size_t & {
            return matrix[i * cols + j];
        };

        for (auto i{0uz}; i <= m; ++i)
            at(i, 0) = i;
        for (auto j{0uz}; j <= n; ++j)
            at(0, j) = j;

        constexpr size_t alphabet_size = static_cast<size_t>(std::numeric_limits<unsigned char>::max()) + 1;
        std::array<size_t, alphabet_size> last_row{};

        const auto chars_equal = [&settings](const unsigned char ac, const unsigned char bc) {
            return ac == bc || (settings.allow_kebab_casing && ((ac == '_' && bc == '-') || (ac == '-' && bc == '_')));
        };

        for (auto i{1uz}; i <= m; ++i) {
            auto last_match_col{0uz};
            const auto ac = static_cast<unsigned char>(a[i - 1]);

            for (auto j{1uz}; j <= n; ++j) {
                const auto bc = static_cast<unsigned char>(b[j - 1]);
                const auto previous_row = last_row[bc];
                const auto previous_col = last_match_col;
                const auto cost = chars_equal(ac, bc) ? 0uz : 1uz;

                if (cost == 0)
                    last_match_col = j;

                auto best = std::min({
                    at(i - 1, j) + 1, // deletion
                    at(i, j - 1) + 1, // insertion
                    at(i - 1, j - 1) + cost // substitution
                });

                if (previous_row != 0 && previous_col != 0) {
                    const auto transposition = at(previous_row - 1, previous_col - 1) + (i - previous_row - 1) + 1 + (j - previous_col - 1);
                    best = std::min(best, transposition);
                }
                at(i, j) = best;
            }
            last_row[ac] = i;
        }
        return at(m, n);
    }

    template<typename T>
    size_t option_max_values(std::string_view name, bool is_short, const Settings &settings) {
        static constexpr auto members = std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        auto n{1uz};
        template for (constexpr auto m : members) {
            using M = [:std::meta::remove_cv(std::meta::type_of(m)):];
            if constexpr (is_named_option<T>(m) && is_container(^^M)) {
                if (does_match_name<m>(name, settings, is_short)) {
                    if constexpr (is_array(^^M))
                        n = std::tuple_size_v<M>;
                    else if constexpr (has_annotation<detail::Nargs_>(m))
                        n = SIZE_MAX;
                }
            }
        }
        return n;
    }

    template<typename T>
    consteval std::optional<std::meta::info> member_named(const char *name) {
        for (const auto m : std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current())) {
            if (name_of(m) == std::string_view{name}) {
                return m;
            }
        }
        return std::nullopt;
    }

    template<typename T>
    consteval std::vector<std::meta::info> group_named(const char *name) {
        static constexpr auto members = std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        std::vector<std::meta::info> result;
        template for (constexpr auto m : members) {
            constexpr auto groups = specialization_of<^^Groups>(m);
            if constexpr (groups.has_value()) {
                static constexpr auto values = get_annotation_list_values<m, ^^Groups>();
                template for (constexpr auto value : values) {
                    if (std::string_view{value} == std::string_view{name}) {
                        result.emplace_back(m);
                    }
                }
            }
        }
        return result;
    }
} // namespace StarParse::detail::Utilities
