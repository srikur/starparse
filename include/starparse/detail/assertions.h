#pragma once

#include <meta>
#include <optional>
#include <algorithm>
#include <vector>

#include <starparse/detail/utilities.hpp>

namespace StarParse::detail::Assertions {
    using namespace StarParse::detail::Utilities;

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

    template<typename T>
    consteval bool no_duplicate_short_names() {
        std::vector<char> short_names;
        for (const auto m : std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current())) {
            const std::optional<Opt> opt = opt_of(m);
            if (opt && opt->short_name > 0) {
                if (std::ranges::find(short_names, opt->short_name) != short_names.end()) return false;
                short_names.push_back(opt->short_name);
            }
        }
        return true;
    }

    template<typename T>
    consteval bool check_positional_indices() {
        const auto members = std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current());
        std::vector<bool> seen(members.size());
        auto count{0uz}, end{0uz};
        for (const auto m : members) {
            if (const auto pos = positional_of(m)) {
                const auto i = pos->index;
                if (i >= seen.size() || seen[i]) return false;
                seen[i] = true;
                count++;
                end = std::max(end, i + 1);
            }
        }
        return count == end;
    }
}
