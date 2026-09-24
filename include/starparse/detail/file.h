#pragma once

#include <print>
#include <expected>
#include <filesystem>
#include <unordered_map>
#include <fstream>

#include "starparse/detail/errors.hpp"


namespace StarParse::detail::File {
    using EnvMap = std::unordered_map<std::string, std::string>;

    [[nodiscard]] inline std::expected<EnvMap, ParseError> read_env(const std::filesystem::path &path) {
        std::ifstream ifs{path, std::ios::binary};
        if (!ifs.is_open()) {
            return std::unexpected(ParseError{
                .kind = ErrorKind::READING_ENV_FAILED,
                .input_value = path.string(),
            });
        }

        const auto trim = [](const std::string_view s) -> std::string_view {
            constexpr std::string_view ws = " \t\r\n\v\f";
            const auto first = s.find_first_not_of(ws);
            if (first == std::string_view::npos) return {};

            const auto last = s.find_last_not_of(ws);
            return s.substr(first, last - first + 1);
        };

        EnvMap result;
        auto line_number{0uz};
        for (std::string storage; std::getline(ifs, storage);) {
            ++line_number;
            std::string_view line{storage};

            if (line_number == 1 && line.starts_with("\xEF\xBB\xBF")) {
                line.remove_prefix(3);
            }

            line = trim(line);
            if (line.empty() || line.front() == '#') continue;

            if (line.starts_with("export ") || line.starts_with("export\t")) {
                line = trim(line.substr(6));
            }

            const auto equals = line.find('=');
            if (equals == std::string_view::npos) {
                return std::unexpected(ParseError{
                    .kind = ErrorKind::INVALID_ENV_VALUE,
                    .input_value = line,
                    .detail = "expected KEY=VALUE",
                    .argv_index = line_number,
                });
            }

            const auto key = trim(line.substr(0, equals));
            auto value = trim(line.substr(equals + 1));
            constexpr std::string_view key_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";

            if (key.empty() || (key.front() >= '0' && key.front() <= '9') ||
                key.find_first_not_of(key_chars) != std::string_view::npos) {
                return std::unexpected(ParseError{
                    .kind = ErrorKind::INVALID_ENV_VALUE,
                    .input_value = line,
                    .detail = "invalid variable name",
                    .argv_index = line_number,
                });
            }

            if (!value.empty() && (value.front() == '\'' || value.front() == '"')) {
                const auto closing = value.find(value.front(), 1);
                if (closing == std::string_view::npos) {
                    return std::unexpected(ParseError{
                        .kind = ErrorKind::INVALID_ENV_VALUE,
                        .input_value = line,
                        .detail = "unterminated quote",
                        .argv_index = line_number,
                    });
                }

                const auto tail = trim(value.substr(closing + 1));
                if (!tail.empty() && tail.front() != '#') {
                    return std::unexpected(ParseError{
                        .kind = ErrorKind::INVALID_ENV_VALUE,
                        .input_value = line,
                        .detail = "unexpected text after quoted value",
                        .argv_index = line_number,
                    });
                }

                value = value.substr(1, closing - 1);
            } else {
                value = trim(value.substr(0, value.find('#')));
                if (value.find_first_of("\"'") != std::string_view::npos) {
                    return std::unexpected(ParseError{
                        .kind = ErrorKind::INVALID_ENV_VALUE,
                        .input_value = line,
                        .detail = "quotes must surround the entire value",
                        .argv_index = line_number,
                    });
                }
            }
            result.insert_or_assign(std::string{key}, std::string{value});
        }

        if (ifs.bad() || (ifs.fail() && !ifs.eof())) {
            return std::unexpected(ParseError{
                .kind = ErrorKind::READING_ENV_FAILED,
                .input_value = path.string(),
            });
        }

        return result;
    }
}
