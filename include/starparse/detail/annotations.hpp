#pragma once

#include <concepts>
#include <string_view>
#include <meta>
#include <string>
#include <array>
#include <expected>
#include <type_traits>

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

        struct Subcommand_ final {};

        struct Required_ final {};
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

    template<typename T>
    concept Numeric = std::integral<T> || std::floating_point<T>;

    template<Numeric T>
    struct Min final {
        T value{};

        explicit consteval Min(const T v) : value(v) {}
    };

    template<Numeric T>
    struct Max final {
        T value{};

        explicit consteval Max(const T v) : value(v) {}
    };

    template<Numeric T>
    struct Range final {
        T min{};
        T max{};
        explicit consteval Range(const T mi, const T ma) : min(mi), max(ma) {}
    };

    template<typename T>
    struct Choices final {
        using value_type = std::conditional_t<std::convertible_to<T, std::string_view>, const char *, T>;
        const value_type *values_{};
        size_t count_{};

        template<std::convertible_to<std::string_view>... Ts>
            requires (sizeof...(Ts) > 0 && std::convertible_to<T, std::string_view>)
        explicit consteval Choices(Ts... ns)
            : values_(std::define_static_array(
                  std::array{std::define_static_string(std::string_view{ns})...}).data()),
              count_(sizeof...(ns)) {}

        template<std::convertible_to<T>... Ts>
            requires (sizeof...(Ts) > 0 && !std::convertible_to<T, std::string_view>)
        explicit consteval Choices(Ts... ns)
            : values_(std::define_static_array(std::array<T, sizeof...(Ts)>{T{ns}...}).data()),
              count_(sizeof...(ns)) {}
    };

    template<typename... Ts>
    Choices(Ts...) -> Choices<std::common_type_t<Ts...> >;

    template<typename T>
    struct Validator final {
        using Result = std::expected<void, std::string>;
        using BoolFn = bool (*)(const T &);
        using CStrFn = const char *(*)(const T &);
        using ExpectedFn = Result (*)(const T &);

        BoolFn bool_fn{};
        CStrFn cstr_fn{};
        ExpectedFn expected_fn{};

        explicit consteval Validator(const BoolFn f) : bool_fn(f) {}
        explicit consteval Validator(const CStrFn f) : cstr_fn(f) {}
        explicit consteval Validator(const ExpectedFn f) : expected_fn(f) {}

        [[nodiscard]] constexpr Result operator()(const T &v) const {
            if (expected_fn) return expected_fn(v);
            if (cstr_fn) {
                if (const char *msg = cstr_fn(v)) {
                    return std::unexpected{std::string{msg}};
                }
            }
            if (bool_fn(v)) return {};
            return std::unexpected{std::string{}};
        }
    };

    template<typename F>
    Validator(F) -> Validator<detail::first_arg_t<F> >;

    constexpr detail::Subcommand_ Subcommand{};

    constexpr detail::Required_ Required{};
}
