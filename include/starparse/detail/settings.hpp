#pragma once

#include <string_view>

namespace StarParse {
    enum class DuplicateOptionPolicy {
        LAST_WINS,
        FIRST_WINS,
        ERROR
    }

    struct Settings {
        bool allow_kebab_casing{true};
        bool allow_aliases{true};
        bool allow_case_insensitivity{true};
        bool autogenerate_negations{true};
        bool allow_repeated_counts{true};
        bool print_help_default_values{true};
        bool infer_arguments{true};
        bool infer_subcommands{false};
        DuplicateOptionPolicy duplicate_option_policy{DuplicateOptionPolicy::LAST_WINS};
        std::string_view value_separator{","};

        Settings &allowKebabCase(const bool value) {
            allow_kebab_casing = value;
            return *this;
        }

        Settings &allowAliases(const bool value) {
            allow_aliases = value;
            return *this;
        }

        Settings &setValueSeparator(const std::string_view value) {
            value_separator = value;
            return *this;
        }

        Settings &allowCaseInsensitivity(const bool value) {
            allow_case_insensitivity = value;
            return *this;
        }

        Settings &autogenerateNegations(const bool value) {
            autogenerate_negations = value;
            return *this;
        }

        Settings &allowRepeatedCounts(const bool value) {
            allow_repeated_counts = value;
            return *this;
        }

        Settings &printHelpDefaultValues(const bool value) {
            print_help_default_values = value;
            return *this;
        }

        Settings &inferArguments(const bool value) {
            infer_arguments = value;
            return *this;
        }

        Settings &inferSubcommands(const bool value) {
            infer_subcommands = value;
            return *this;
        }
    };
} // namespace StarParse
