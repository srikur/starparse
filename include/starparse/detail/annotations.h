#pragma once

#include <concepts>
#include <string_view>
#include <meta>
#include <string>
#include <array>

namespace detail::StarParse::Annotations {
    struct Opt final {
        char short_name{0};
        const char *help_{};

        explicit consteval Opt(const char s, std::string_view h) : short_name(s), help_(std::define_static_string(h)) {}

        explicit consteval Opt(std::string_view h) : help_(std::define_static_string(h)) {}

        explicit consteval Opt(const char s) : short_name(s) {}

        [[nodiscard]] constexpr std::string_view help() const { return help_; }
    };

    struct Positional final {
        size_t index;
    };

    struct Required final {};

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
        Program(std::string_view name, std::string_view description, std::string_view version) : name_(
                std::define_static_string(name)), description_(std::define_static_string(description)),
            version_(std::define_static_string(version)) {}

        const char *name_{};
        const char *description_{};
        const char *version_{};
    };
}
