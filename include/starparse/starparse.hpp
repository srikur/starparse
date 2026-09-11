#pragma once

#include <algorithm>
#include <ranges>
#include <expected>
#include <print>
#include <cstdlib>

#include <starparse/detail/parser.hpp>
#include <starparse/detail/settings.hpp>

namespace StarParse {
    template<typename T>
    std::expected<T, std::vector<detail::ParseError> > try_parse(const int argc, char **argv, T initial = {},
                                                                 const detail::Settings settings = {}) {
        auto result = detail::Parser::parse<T>(argc, argv, std::move(initial), settings);
        if (result.help_requested()) {
            std::print("{}", result.help());
            std::exit(EXIT_SUCCESS);
        }
        if (result.version_requested()) {
            std::print("{}", result.version());
            std::exit(EXIT_SUCCESS);
        }
        if (result.errors().empty()) {
            return std::move(result).value();
        }
        return std::unexpected(std::vector<detail::ParseError>(result.errors().begin(), result.errors().end()));
    }

    template<typename T>
    T parse_or_exit(const int argc, char **argv, T initial = {}, const detail::Settings settings = {}) {
        auto result = detail::Parser::parse<T>(argc, argv, std::move(initial), settings);
        if (result.help_requested()) {
            std::print("{}", result.help());
            std::exit(EXIT_SUCCESS);
        }
        if (result.version_requested()) {
            std::print("{}", result.version());
            std::exit(EXIT_SUCCESS);
        }
        if (!result.errors().empty()) {
            std::println("Error: {}", result.errors()[0]);
            std::exit(EXIT_FAILURE);
        }
        return std::move(result).value();
    }

    template<typename T>
    T force_parse(const int argc, char **argv, T initial = {}, const detail::Settings settings = {}) {
        auto result = detail::Parser::parse<T>(argc, argv, std::move(initial), settings);
        if (result.help_requested()) {
            std::print("{}", result.help());
            std::exit(EXIT_SUCCESS);
        }
        if (result.version_requested()) {
            std::print("{}", result.version());
            std::exit(EXIT_SUCCESS);
        }
        if (!result.errors().empty()) {
            throw std::invalid_argument(result.errors()[0].to_string());
        }
        return std::move(result).value();
    }
}
