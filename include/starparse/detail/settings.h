#pragma once

namespace detail::StarParse {
    struct Settings {
        bool allow_kebab_casing{true};
        bool allow_upper_casing{true};
        bool allow_aliases{true};

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
    };
}
