#pragma once

#include <algorithm>
#include <cstdlib>
#include <initializer_list>
#include <print>
#include <ranges>

#include <starparse/detail/parser.hpp>
#include <starparse/detail/settings.hpp>

namespace StarParse {
    using detail::Parser::ParsedArgs;

    template<typename T, Settings settings = Settings{}>
    ParsedArgs<T, settings> parse(const int argc, const char *const *argv, T initial = {}) {
        std::vector<std::string_view> args;
        if (argc > 1) {
            args.assign(argv + 1, argv + argc);
        }
        return detail::Parser::parse<T, settings>(args, std::string_view{argv[0]}, std::move(initial));
    }

    template<typename T, Settings settings = Settings{}>
    ParsedArgs<T, settings> parse_or_exit(const int argc, const char *const *argv, T initial = {}) {
        std::vector<std::string_view> args;
        if (argc > 1) {
            args.assign(argv + 1, argv + argc);
        }
        auto result = detail::Parser::parse<T, settings>(args, std::string_view{argv[0]}, std::move(initial));
        if (!result.errors().empty()) {
            for (const auto &error : result.errors()) {
                std::println(stderr, "Error: {}", error.to_string());
            }
            std::exit(EXIT_FAILURE);
        }
        return result;
    }

    template<typename T, Settings settings = Settings{}>
    ParsedArgs<T, settings> parse_or_throw(const int argc, const char *const *argv, T initial = {}) {
        std::vector<std::string_view> args;
        if (argc > 1) {
            args.assign(argv + 1, argv + argc);
        }
        auto result = detail::Parser::parse<T, settings>(args, std::string_view{argv[0]}, std::move(initial));
        if (!result.errors().empty()) {
            throw std::invalid_argument(result.errors()[0].to_string());
        }
        return result;
    }

    template<typename T, Settings settings = Settings{}>
    ParsedArgs<T, settings> parse_from(const std::initializer_list<std::string_view> args, T initial = {}) {
        return detail::Parser::parse<T, settings>(std::span{args.begin(), args.size()}, "", std::move(initial));
    }
} // namespace StarParse
