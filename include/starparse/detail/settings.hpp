#pragma once

#include <string_view>

namespace StarParse::detail {
    struct Settings {
        bool allow_kebab_casing{true};
        bool allow_upper_casing{true};
        bool allow_aliases{true};
        std::string_view value_separator{","};

        Settings &setAllowKebabCasing(const bool value) {
            allow_kebab_casing = value;
            return *this;
        }

        Settings &setAllowUpperCasing(const bool value) {
            allow_upper_casing = value;
            return *this;
        }

        Settings &setAllowAliases(const bool value) {
            allow_aliases = value;
            return *this;
        }

        Settings &setValueSeparator(const std::string_view value) {
            value_separator = value;
            return *this;
        }
    };
}
