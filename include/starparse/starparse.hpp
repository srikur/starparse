#pragma once

#include <algorithm>
#include <ranges>
#include <expected>

#include <starparse/detail/parser.hpp>
#include <starparse/detail/settings.hpp>

namespace StarParse {
    template<typename T>
    std::expected<T, std::vector<detail::ParseError> > try_parse(const int argc, char **argv, T initial = {},
                                                                 const detail::Settings settings = {}) {
        try {
            auto result = detail::Parser::parse<T>(argc, argv, std::move(initial), settings);
            return result.errors().size() ? result.errors() : std::move(result).value();
        } catch ([[maybe_unused]] const std::exception &e) {
            return std::vector{detail::ParseError{.kind = detail::ErrorKind::EXCEPTION}};
        }
    }

    template<typename T>
    T parse_or_exit(const int argc, char **argv, T initial = {}, const detail::Settings settings = {}) {
        try {
            auto result = detail::Parser::parse<T>(argc, argv, std::move(initial), settings);
            return std::move(result).value();
        } catch ([[maybe_unused]] const std::exception &e) {
            std::terminate();
        }
    }

    template<typename T>
    T force_parse(const int argc, char **argv, T initial = {}, const detail::Settings settings = {}) {
        auto result = detail::Parser::parse<T>(argc, argv, std::move(initial), settings);
        return std::move(result).value();
    }
}
