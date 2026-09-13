#pragma once

#include <algorithm>
#include <ranges>
#include <expected>
#include <print>
#include <cstdlib>

#include <starparse/detail/parser.hpp>
#include <starparse/detail/settings.hpp>

namespace StarParse {
    using detail::Parser::ParsedArgs;

    template<typename T>
    ParsedArgs<T> parse(const int argc, char **argv, T initial = {},
                        const Settings settings = {}) {
        auto result = detail::Parser::parse<T>(argc, argv, std::move(initial), settings);
        return std::move(result);
    }

    template<typename T>
    ParsedArgs<T> parse_or_exit(const int argc, char **argv, T initial = {}, const Settings settings = {}) {
        auto result = detail::Parser::parse<T>(argc, argv, std::move(initial), settings);
        if (!result.errors().empty()) {
            for (const auto& error : result.errors()) {
                std::println(stderr, "Error: {}", error.to_string());
            }
            std::exit(EXIT_FAILURE);
        }
        return std::move(result);
    }

    template<typename T>
    ParsedArgs<T> parse_or_throw(const int argc, char **argv, T initial = {},
                                 const Settings settings = {}) {
        auto result = detail::Parser::parse<T>(argc, argv, std::move(initial), settings);
        if (!result.errors().empty()) {
            throw std::invalid_argument(result.errors()[0].to_string());
        }
        return std::move(result);
    }
}
