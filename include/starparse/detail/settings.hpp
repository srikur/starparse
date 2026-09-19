#pragma once

#include <string_view>

namespace StarParse {
    struct Settings {
        bool allow_kebab_casing{true};
        bool allow_aliases{true};
        bool allow_case_insensitivity{true};
        bool autogenerate_negations{true};
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
    };
}
