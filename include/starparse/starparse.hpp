#pragma once

#include <algorithm>
#include <ranges>
#include <expected>

#include <starparse/detail/parser.h>
#include <starparse/detail/settings.h>

namespace StarParse {
    using detail::StarParse::Settings;
    using namespace detail::StarParse::Parser;

    template<typename T>
    std::expected<T, std::vector<ParseError> > try_parse(const int argc, char **argv, T initial = {},
                                                         const Settings settings = {}) {
        try {
            auto result = parse<T>(argc, argv, std::move(initial), settings);
            return result.errors().size() ? result.errors() : std::move(result).value();
        } catch ([[maybe_unused]] const std::exception &e) {
            return std::vector{ParseError{.kind = ErrorKind::EXCEPTION}};
        }
    }

    template<typename T>
    T parse_or_exit(const int argc, char **argv, T initial = {}, const Settings settings = {}) {
        try {
            auto result = parse<T>(argc, argv, std::move(initial), settings);
            return std::move(result).value();
        } catch ([[maybe_unused]] const std::exception &e) {
            std::terminate();
        }
    }

    template<typename T>
    T force_parse(const int argc, char **argv, T initial = {}, const Settings settings = {}) {
        auto result = parse<T>(argc, argv, std::move(initial), settings);
        return std::move(result).value();
    }
}
