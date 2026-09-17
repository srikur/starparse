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
}
