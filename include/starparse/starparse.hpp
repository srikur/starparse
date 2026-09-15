#pragma once

#include <algorithm>
#include <ranges>
#include <initializer_list>
#include <print>
#include <cstdlib>

#include <starparse/detail/parser.hpp>
#include <starparse/detail/settings.hpp>

namespace StarParse {
    using detail::Parser::ParsedArgs;

    template<typename T>
    ParsedArgs<T> parse(const int argc, const char *const*argv, const Settings settings) {
        return parse<T>(argc, argv, T{}, settings);
    }

    template<typename T>
    ParsedArgs<T> parse(const int argc, const char *const*argv, T initial = {},
                        const Settings settings = {}) {
        std::vector<std::string_view> args;
        if (argc > 1) {
            args.assign(argv + 1, argv + argc);
        }
        return detail::Parser::parse<T>(args, std::move(initial), settings);
    }

    template<typename T>
    ParsedArgs<T> parse_or_exit(const int argc, const char *const*argv, T initial = {}, const Settings settings = {}) {
        std::vector<std::string_view> args;
        if (argc > 1) {
            args.assign(argv + 1, argv + argc);
        }
        auto result = detail::Parser::parse<T>(args, std::move(initial), settings);
        if (!result.errors().empty()) {
            for (const auto &error : result.errors()) {
                std::println(stderr, "Error: {}", error.to_string());
            }
            std::exit(EXIT_FAILURE);
        }
        return result;
    }

    template<typename T>
    ParsedArgs<T> parse_or_throw(const int argc, const char *const*argv, T initial = {},
                                 const Settings settings = {}) {
        std::vector<std::string_view> args;
        if (argc > 1) {
            args.assign(argv + 1, argv + argc);
        }
        auto result = detail::Parser::parse<T>(args, std::move(initial), settings);
        if (!result.errors().empty()) {
            throw std::invalid_argument(result.errors()[0].to_string());
        }
        return result;
    }

    template<typename T>
    ParsedArgs<T> parse_from(std::initializer_list<std::string_view> args, T initial = {},
                             const Settings settings = {}) {
        return detail::Parser::parse<T>(std::span{args.begin(), args.size()}, std::move(initial), settings);
    }
}
