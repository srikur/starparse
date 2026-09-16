#pragma once

#include <string_view>
#include <string>
#include <optional>
#include <format>

namespace StarParse {
    enum class ErrorKind {
        UNKNOWN_OPTION, MISSING_VALUE, INVALID_VALUE, UNEXPECTED_POSITIONAL, MISSING_REQUIRED, DUPLICATE_OPTION,
        EXCEPTION, OUT_OF_RANGE, INVALID_CHOICE, VALIDATION_FAILED
    };

    struct ParseError {
        ErrorKind kind;
        std::string_view input_value{};
        std::string detail{};
        std::optional<std::string_view> current_argument{std::nullopt};
        size_t argv_index{};

        [[nodiscard]] std::string to_string() const {
            const std::string_view option = current_argument.value_or("");
            switch (kind) {
                case ErrorKind::UNKNOWN_OPTION:
                    return std::format("Unknown option '{}'", input_value);
                case ErrorKind::MISSING_VALUE:
                    return std::format("Missing value for argument '{}'", option);
                case ErrorKind::INVALID_VALUE:
                    return std::format("Could not parse input '{}' for argument '{}'", input_value, option);
                case ErrorKind::UNEXPECTED_POSITIONAL:
                    return std::format("Unexpected positional value '{}'", input_value);
                case ErrorKind::MISSING_REQUIRED:
                    return std::format("Missing value for required option '{}'", option);
                case ErrorKind::DUPLICATE_OPTION:
                    return std::format("Duplicate value '{}' provided for option '{}'", input_value, option);
                case ErrorKind::EXCEPTION:
                    return std::format("Exception thrown while validating input '{}': {}", input_value, detail);
                case ErrorKind::OUT_OF_RANGE:
                    return std::format("Value {} is out of range for argument '{}'; {}", input_value, option, detail);
                case ErrorKind::INVALID_CHOICE:
                    return std::format("Invalid choice '{}' for argument '{}'; allowed choices are {}", input_value, option, detail);
                case ErrorKind::VALIDATION_FAILED:
                    return std::format("Validation failed for input '{}': {}", input_value, detail);
                default:
                    return std::format("Unknown error: '{}'", input_value);
            }
        }
    };
}

template<>
struct std::formatter<StarParse::ParseError> : std::formatter<std::string> {
    template<typename FormatContext>
    auto format(const StarParse::ParseError &e, FormatContext &ctx) const {
        return std::formatter<std::string>::format(e.to_string(), ctx);
    }
};
