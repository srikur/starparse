#pragma once

#include <string_view>
#include <format>

namespace StarParse::detail {
    enum class ErrorKind {
        UNKNOWN_OPTION, MISSING_VALUE, INVALID_VALUE, UNEXPECTED_POSITIONAL, MISSING_REQUIRED, DUPLICATE_OPTION,
        EXCEPTION
    };

    constexpr std::string_view error_kind_string(const StarParse::detail::ErrorKind k) {
        switch (k) {
                using StarParse::detail::ErrorKind;
            case ErrorKind::UNKNOWN_OPTION: return "Unknown option";
            case ErrorKind::MISSING_VALUE: return "Missing value";
            case ErrorKind::INVALID_VALUE: return "Invalid value";
            case ErrorKind::UNEXPECTED_POSITIONAL: return "Unexpected positional";
            case ErrorKind::MISSING_REQUIRED: return "Missing required";
            case ErrorKind::DUPLICATE_OPTION: return "Duplicate option";
            case ErrorKind::EXCEPTION: return "Exception";
        }
        return "?";
    }
}

template<>
struct std::formatter<StarParse::detail::ErrorKind> : std::formatter<std::string_view> {
    auto format(const StarParse::detail::ErrorKind k, std::format_context &ctx) const {
        return std::formatter<std::string_view>::format(error_kind_string(k), ctx);
    }
};

namespace StarParse::detail {
    struct ParseError {
        ErrorKind kind;
        std::string_view token;
        std::string_view option;
        int argv_index;

        [[nodiscard]] std::string to_string() const {
            return std::format("{} supplied for argument {} (value: {}) at position {}.", kind, option,
                               token.empty() ? "none" : token, argv_index);
        }
    };
}

template<>
struct std::formatter<StarParse::detail::ParseError> : std::formatter<std::string> {
    template<typename FormatContext>
    auto format(const StarParse::detail::ParseError &e, FormatContext &ctx) const {
        return std::formatter<std::string>::format(e.to_string(), ctx);
    }
};
