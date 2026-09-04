#pragma once

#include <concepts>
#include <optional>
#include <string_view>
#include <meta>
#include <string>
#include <stdexcept>
#include <charconv>
#include <print>
#include <type_traits>
#include <array>
#include <vector>

#include "annotations.hpp"

namespace StarParse::detail::Utilities {
    using Annotations::Alias;
    using Annotations::Positional;
    using Annotations::Opt;
    using Annotations::Program;
    using Annotations::Required;

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

    consteval bool is_specialization_of(std::meta::info type, const std::meta::info templ) {
        type = std::meta::dealias(type);
        return std::meta::has_template_arguments(type) && std::meta::template_of(type) == templ;
    }

    consteval bool is_vector(const std::meta::info r) { return is_specialization_of(r, ^^std::vector); }
    consteval bool is_array(const std::meta::info r) { return is_specialization_of(r, ^^std::array); }
    consteval bool is_container(const std::meta::info m) { return is_vector(m) || is_array(m); }

    constexpr bool bool_from_string(const std::string_view s) {
        if (s.size() == 1) return s[0] == 'T' || s[0] == 't';
        return s == "True" || s == "true";
    }

    template<typename M>
    M from_string(std::string_view s) {
        if constexpr (is_optional(^^M)) {
            using T = [:value_type_of(^^M):];
            return M{from_string<T>(s)};
        } else if constexpr (std::same_as<M, bool>) {
            return M{bool_from_string(s)};
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
        for (const auto a : std::meta::annotations_of(m)) {
            if (std::meta::dealias(std::meta::remove_cv(std::meta::type_of(a))) == std::meta::dealias(^^Required)) {
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

    template<typename T>
    consteval bool no_required_optionals() {
        for (const auto m : std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current())) {
            if (is_required(m) && is_optional(std::meta::remove_cv(std::meta::type_of(m)))) {
                return false;
            }
        }
        return true;
    }

    template<typename T>
    consteval bool no_positional_containers() {
        for (const auto m : std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current())) {
            if (is_positional(m) && is_container(std::meta::remove_cv(std::meta::type_of(m)))) {
                return false;
            }
        }
        return true;
    }

    template<typename M>
    void assign_from_string(M &field, const std::string_view s, const bool first_occurrence) {
        if constexpr (is_vector(std::meta::remove_cv(^^M))) {
            using E = [:value_type_of(^^M):];
            if (first_occurrence) field.clear();
            field.push_back(from_string<E>(s));
        } else {
            field = from_string<M>(s);
        }
    }
}
