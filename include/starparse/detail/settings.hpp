#pragma once

namespace StarParse {
    enum class DuplicateOptionPolicy {
        LAST_WINS,
        FIRST_WINS,
        ERROR
    };

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
        unsigned char value_separator{','};
    };
} // namespace StarParse
