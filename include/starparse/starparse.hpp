#pragma once

#include <algorithm>
#include <ranges>

#include <starparse/detail/parser.h>
#include <starparse/detail/settings.h>

namespace StarParse {
    using detail::StarParse::Settings;
    using namespace detail::StarParse::Parser;

    template<typename T>
    T force_parse(const int argc, char **argv, T initial = {}, const Settings settings = {}) {
        auto result = parse<T>(argc, argv, std::move(initial), settings);
        return std::move(result).value();
    }
}
