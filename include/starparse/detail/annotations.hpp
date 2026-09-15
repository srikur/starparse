#pragma once

#include <concepts>
#include <string_view>
#include <meta>
#include <string>
#include <array>

namespace StarParse::inline annotations {
    namespace detail {
        template<typename F>
        struct first_arg : first_arg<decltype(&F::operator())> {};

        template<typename R, typename C, typename A>
        struct first_arg<R (C::*)(A) const> {
            using type = std::remove_cvref_t<A>;
        };

        template<typename R, typename C, typename A>
        struct first_arg<R (C::*)(A)> {
            using type = std::remove_cvref_t<A>;
        };

        template<typename R, typename A>
        struct first_arg<R (*)(A)> {
            using type = std::remove_cvref_t<A>;
        };

        template<typename R, typename A>
        struct first_arg<R (A)> {
            using type = std::remove_cvref_t<A>;
        };

        template<typename F>
        using first_arg_t = first_arg<std::remove_cvref_t<F> >::type;
    }

    struct Opt final {
        char short_name{0};
        const char *help_{};

        explicit consteval Opt(const char s, std::string_view h) : short_name(s),
                                                                   help_(std::define_static_string(h)) {}

        explicit consteval Opt(std::string_view h) : help_(std::define_static_string(h)) {}

        explicit consteval Opt(const char s) : short_name(s) {}

        [[nodiscard]] constexpr std::string_view help() const { return help_; }
    };

    struct Positional final {
        size_t index{};
        const char *help_{};

        explicit consteval
        Positional(const size_t i, std::string_view h) : index(i), help_(std::define_static_string(h)) {}

        explicit consteval Positional(const size_t i) : index(i) {}

        [[nodiscard]] constexpr std::string_view help() const { return help_; }
    };

    struct Required final {};

    struct Separator final {
        const char *value{};

        explicit consteval Separator(std::string_view s) : value(std::define_static_string(s)) {}
    };

    struct Alias final {
        const char *const*names_{};
        size_t count_{};

        template<std::convertible_to<std::string_view>... Ts>
            requires (sizeof...(Ts) > 0)
        explicit consteval Alias(Ts... ns)
            : names_(std::define_static_array(
                  std::array{std::define_static_string(std::string_view{ns})...}).data()),
              count_(sizeof...(ns)) {}
    };

    struct Program final {
        explicit consteval
        Program(std::string_view n, std::string_view d, std::string_view v) : name(std::define_static_string(n)),
                                                                              description(std::define_static_string(d)),
                                                                              version(std::define_static_string(v)) {}

        const char *name{};
        const char *description{};
        const char *version{};
    };

    struct Min final {
        double value{};

        explicit consteval Min(const double v) : value(v) {}
    };

    struct Max final {
        double value{};

        explicit consteval Max(const double v) : value(v) {}
    };

    struct Choices final {
        const char *const*names_{};
        size_t count_{};

        template<std::convertible_to<std::string_view>... Ts>
            requires (sizeof...(Ts) > 0)
        explicit consteval Choices(Ts... ns)
            : names_(std::define_static_array(
                  std::array{std::define_static_string(std::string_view{ns})...}).data()),
              count_(sizeof...(ns)) {}
    };

    template<typename T>
    struct Validator final {
        using Fn = bool (*)(const T &);
        Fn fn{};

        explicit consteval Validator(const Fn f) : fn(f) {}

        template<std::convertible_to<Fn> F>
        explicit consteval Validator(F f) : fn(static_cast<Fn>(f)) {}

        [[nodiscard]] constexpr bool operator()(const T &v) const { return fn(v); }
    };

    template<typename F>
    Validator(F) -> Validator<detail::first_arg_t<F> >;
}
