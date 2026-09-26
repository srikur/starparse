#pragma once

#include <meta>
#include <optional>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <starparse/detail/utilities.hpp>

namespace StarParse::detail::Assertions {
    using namespace StarParse::detail::Utilities;

    enum class AnnotationKind {
        UNKNOWN, OPT, POSITIONAL, SUBCOMMAND, REQUIRED, SEPARATOR, ALIAS, NAME, PROGRAM, MIN, MAX, RANGE, CHOICES, VALIDATOR, COUNT
    };

    consteval AnnotationKind annotation_kind(const std::meta::info annotation) {
        const auto type = std::meta::dealias(std::meta::remove_cv(std::meta::type_of(annotation)));
        if (type == ^^Opt) return AnnotationKind::OPT;
        if (type == ^^Positional) return AnnotationKind::POSITIONAL;
        if (type == ^^detail::Subcommand_) return AnnotationKind::SUBCOMMAND;
        if (type == ^^detail::Required_) return AnnotationKind::REQUIRED;
        if (type == ^^Separator) return AnnotationKind::SEPARATOR;
        if (type == ^^Alias) return AnnotationKind::ALIAS;
        if (type == ^^Name) return AnnotationKind::NAME;
        if (type == ^^Program) return AnnotationKind::PROGRAM;
        if (is_specialization_of(type, ^^Min)) return AnnotationKind::MIN;
        if (is_specialization_of(type, ^^Max)) return AnnotationKind::MAX;
        if (is_specialization_of(type, ^^Range)) return AnnotationKind::RANGE;
        if (is_specialization_of(type, ^^Choices)) return AnnotationKind::CHOICES;
        if (is_specialization_of(type, ^^Validator)) return AnnotationKind::VALIDATOR;
        return AnnotationKind::UNKNOWN;
    }

    consteval bool has_parser_annotations(const std::meta::info entity) {
        for (const auto annotation : std::meta::annotations_of(entity)) {
            if (annotation_kind(annotation) != AnnotationKind::UNKNOWN) return true;
        }
        return false;
    }

    consteval std::meta::info parsed_value_type(const std::meta::info member) {
        auto type = std::meta::dealias(std::meta::remove_cv(std::meta::type_of(member)));
        while (is_optional(type) || is_container(type)) {
            type = std::meta::dealias(std::meta::remove_cv(value_type_of(type)));
        }
        return type;
    }

    template<typename T>
    consteval bool check_annotation_placement() {
        for (const auto annotation : std::meta::annotations_of(^^T)) {
            const auto kind = annotation_kind(annotation);
            if (kind != AnnotationKind::UNKNOWN && kind != AnnotationKind::PROGRAM) return false;
        }
        for (const auto member : std::meta::members_of(^^T, std::meta::access_context::current())) {
            if (std::meta::is_type(member)) continue;
            if (!std::meta::is_nonstatic_data_member(member)) {
                if (has_parser_annotations(member)) return false;
                continue;
            }
            if (program_of(member).has_value()) return false;
            const auto type = parsed_value_type(member);
            if (std::meta::is_enum_type(type)) {
                if (has_parser_annotations(type)) return false;
                for (const auto enumerator : std::meta::enumerators_of(type)) {
                    for (const auto annotation : std::meta::annotations_of(enumerator)) {
                        const auto kind = annotation_kind(annotation);
                        if (kind != AnnotationKind::UNKNOWN && kind != AnnotationKind::ALIAS) return false;
                    }
                }
            }
        }
        return true;
    }

    template<typename T>
    consteval bool no_annotations_on_ignored_fields() {
        for (const auto member : std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current())) {
            if (!is_named_option<T>(member) && !is_subcommand(member) && has_parser_annotations(member)) return false;
        }
        return true;
    }

    consteval bool unique_annotations(const std::meta::info entity) {
        std::array<size_t, static_cast<size_t>(AnnotationKind::COUNT)> counts{};
        for (const auto annotation : std::meta::annotations_of(entity)) {
            const auto kind = annotation_kind(annotation);
            if (kind == AnnotationKind::UNKNOWN || kind == AnnotationKind::ALIAS) continue;
            if (++counts[static_cast<size_t>(kind)] > 1) return false;
        }
        return true;
    }

    template<typename T>
    consteval bool no_duplicate_annotations() {
        if (!unique_annotations(^^T)) return false;
        for (const auto member : std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current())) {
            if (!unique_annotations(member)) return false;
        }
        return true;
    }

    constexpr bool valid_alias(const std::string_view name) {
        if (name.empty() || name.front() == '-') return false;
        return name.find_first_of("= \t\n\r\v\f") == std::string_view::npos;
    }

    consteval bool valid_names(const std::meta::info entity) {
        for (const auto annotation : std::meta::annotations_of(entity)) {
            const auto kind = annotation_kind(annotation);
            if (kind == AnnotationKind::OPT) {
                const auto opt = std::meta::extract<Opt>(annotation);
                const auto c = static_cast<unsigned char>(opt.short_name);
                // zero means no short name
                if (c != 0 && (c <= ' ' || c == 127 || c == '-' || c == '=')) return false;
            } else if (kind == AnnotationKind::ALIAS) {
                const auto alias = std::meta::extract<Alias>(annotation);
                for (size_t i = 0; i < alias.count_; ++i) {
                    if (alias.names_[i] == nullptr || !valid_alias(alias.names_[i])) return false;
                }
            } else if (kind == AnnotationKind::NAME) {
                const auto name = std::meta::extract<Name>(annotation);
                if (name.name_ == nullptr || !valid_alias(name.name_)) return false;
            }
        }
        return true;
    }

    template<typename T>
    consteval bool check_name_values() {
        for (const auto member : std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current())) {
            if (!valid_names(member)) return false;
            const auto type = parsed_value_type(member);
            if (std::meta::is_enum_type(type)) {
                for (const auto enumerator : std::meta::enumerators_of(type)) {
                    if (!valid_names(enumerator)) return false;
                }
            }
        }
        return true;
    }

    struct Name {
        std::string text;
        std::meta::info owner;
        bool is_short{};
        bool is_alias{};
    };

    consteval void append_names(std::vector<Name> &names, const std::meta::info entity,
                                const bool include_short_name = true) {
        const std::string name{name_of(entity)};
        names.push_back({.text = name, .owner = entity});
        std::string kebab = name;
        std::ranges::replace(kebab, '_', '-');
        if (kebab != name) names.push_back({.text = std::move(kebab), .owner = entity});
        if (const auto opt = opt_of(entity); include_short_name && opt && opt->short_name != 0) {
            names.push_back({.text = std::string(1, opt->short_name), .owner = entity, .is_short = true});
        }
        for (const char *alias : alias_name_list(entity)) {
            if (alias != nullptr) {
                names.push_back({.text = std::string{alias}, .owner = entity, .is_short = false, .is_alias = true});
            }
        }
    }

    consteval bool unique_names(const std::vector<Name> &names, const bool reserve_builtin_options) {
        for (auto i{0uz}; i < names.size(); ++i) {
            const auto &name = names[i];
            if (reserve_builtin_options && (same_name(name.text, "help") || same_name(name.text, "version"))) {
                return false;
            }
            for (auto j{0uz}; j < i; ++j) {
                const auto &other = names[j];
                if (!same_name(name.text, other.text, name.is_short && other.is_short)) continue;
                // option's own short name can repeat its one-character field name
                if (name.owner == other.owner && !name.is_alias && !other.is_alias) continue;
                return false;
            }
        }
        return true;
    }

    template<typename T>
    consteval bool check_alias_collisions() {
        std::vector<Name> names;
        const auto members = std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current());
        for (const auto m : members) {
            if (is_named_option<T>(m)) append_names(names, m);
        }
        return unique_names(names, true);
    }

    template<typename T>
    consteval bool check_subcommand_collisions() {
        std::vector<Name> names;
        for (const auto m : std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current())) {
            if (is_subcommand(m)) append_names(names, m, false);
        }
        return unique_names(names, false);
    }

    template<typename T>
    consteval bool check_enum_alias_collisions() {
        const auto members = std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current());
        for (const auto m : members) {
            if (!is_named_option<T>(m)) continue;
            const auto type = parsed_value_type(m);
            if (!std::meta::is_enum_type(type)) continue;
            std::vector<Name> names;
            for (const auto enumerator : std::meta::enumerators_of(type)) append_names(names, enumerator);
            if (!unique_names(names, false)) return false;
        }
        return true;
    }

    template<typename T>
    consteval bool no_required_optionals() {
        const auto members = std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current());
        for (const auto m : members) {
            if (is_required(m) && is_optional(std::meta::remove_cv(std::meta::type_of(m)))) {
                return false;
            }
        }
        return true;
    }

    template<typename T>
    consteval bool no_duplicate_short_names() {
        std::vector<char> short_names;
        const auto members = std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current());
        for (const auto m : members) {
            const std::optional<Opt> opt = opt_of(m);
            if (opt && opt->short_name > 0) {
                if (std::ranges::find(short_names, opt->short_name) != short_names.end()) return false;
                short_names.push_back(opt->short_name);
            }
        }
        return true;
    }

    template<typename T>
    consteval std::pair<ssize_t, ssize_t> get_positional_range() {
        const auto members = std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current());
        std::vector<bool> seen(members.size());
        auto count{0uZ}, end{0uZ};
        for (const auto m : members) {
            if (const auto pos = positional_of(m)) {
                const auto i = pos->index;
                if (i >= seen.size() || seen[i]) return {-1, 0};
                seen[i] = true;
                count++;
                end = std::max(end, i + 1);
            }
        }
        return {count, end};
    }

    template<typename T>
    consteval bool check_positional_indices() {
        const auto [count, end] = get_positional_range<T>();
        return count != -1 && count == end; // -1 used as sentinel to represent false
    }

    template<typename T>
    consteval bool no_positional_containers() {
        const auto members = std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current());
        const auto [count, end] = get_positional_range<T>();
        for (const auto m : members) {
            if (const auto pos = positional_of(m); pos && is_container(std::meta::remove_cv(std::meta::type_of(m)))) {
                if (static_cast<ssize_t>(pos->index) != end - 1) return false;
            }
        }
        return true;
    }

    template<typename T>
    consteval bool no_duplicate_validators() {
        const auto members = std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current());
        for (const auto m : members) {
            const std::vector<std::meta::info> annotations = std::meta::annotations_of(m);
            const auto validator_count = std::ranges::count_if(annotations, [](const std::meta::info a) {
                return is_specialization_of(std::meta::remove_cv(std::meta::type_of(a)), ^^Validator);
            });
            if (validator_count > 1) return false;
            const auto min_count = std::ranges::count_if(annotations, [](const std::meta::info a) {
                return is_specialization_of(std::meta::remove_cv(std::meta::type_of(a)), ^^Min);
            });
            if (min_count > 1) return false;
            const auto max_count = std::ranges::count_if(annotations, [](const std::meta::info a) {
                return is_specialization_of(std::meta::remove_cv(std::meta::type_of(a)), ^^Max);
            });
            if (max_count > 1) return false;
            const auto range_count = std::ranges::count_if(annotations, [](const std::meta::info a) {
                return is_specialization_of(std::meta::remove_cv(std::meta::type_of(a)), ^^Range);
            });
            if (range_count > 1) return false;
            const auto choices_count = std::ranges::count_if(annotations, [](const std::meta::info a) {
                return is_specialization_of(std::meta::remove_cv(std::meta::type_of(a)), ^^Choices);
            });
            if (choices_count > 1) return false;
            // also if more than 1 on a field
            if (validator_count + min_count + max_count + range_count + choices_count > 1) return false;
        }
        return true;
    }

    template<typename T>
    consteval bool no_scalar_separators() {
        const auto members = std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current());
        for (const auto m : members) {
            if (!is_container(std::meta::remove_cv(std::meta::type_of(m))) && separator_of(m).has_value()) return false;
        }
        return true;
    }

    template<typename T>
    consteval bool check_ranges() {
        static constexpr auto members = std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        template for (constexpr auto m : members) {
            constexpr auto range_annotation = range_of(m);

            if constexpr (range_annotation.has_value()) {
                using A = [:std::meta::remove_cv(std::meta::type_of(*range_annotation)):];
                const auto range = std::meta::extract<A>(*range_annotation);
                // 1) max > min
                if (numeric_less(range.max, range.min) || range.min == range.max) return false;
                // 2) check for NaN/infinite
                using V = [:std::meta::remove_cv(std::meta::type_of(m)):];
                if constexpr (std::floating_point<V>) {
                    if (!std::isfinite(range.max) || !std::isfinite(range.min)) return false;
                }
                // 3) Check for no in-range values possible for specified type
                if (range.min < std::numeric_limits<V>::lowest() || range.min > std::numeric_limits<V>::max()) return false;
                if (range.max < std::numeric_limits<V>::lowest() || range.max > std::numeric_limits<V>::max()) return false;
            }
        }
        return true;
    }
}
