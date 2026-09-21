#include "test_support.hpp"

#include <cstdint>

using namespace StarParse;

namespace {
    // Models from StarGBC
    enum class Model : std::uint8_t {
        Auto,
        DMG0, DMGA, DMGB, DMGC,
        MGB, SGB, SGB2,
        CGB0, CGBA, CGBB, CGBC, CGBD, CGBE,
        AGB0, AGBA, AGBAE, AGBB, AGBBE,
    };

    struct ModelArgs {
        [[=Opt{'m', "Model"}]] Model model{Model::Auto};
    };

    struct KebabArgs {
        int dry_run;
    };
}

TEST_CASE("settings: allow_case_insensitivity") {
    SUBCASE("enabled: enum values match in any case") {
        const auto args = parse_from<ModelArgs>({"--model=cgb0"}, Settings{.allow_case_insensitivity = true});
        REQUIRE(args);
        CHECK(args->model == Model::CGB0);
    }
    SUBCASE("disabled: enum values must match exactly") {
        const auto args = parse_from<ModelArgs>({"--model=cgb0"}, Settings{.allow_case_insensitivity = false});
        REQUIRE_FALSE(args);
        CHECK(args.errors()[0].kind == ErrorKind::INVALID_VALUE);
        const auto exact = parse_from<ModelArgs>({"--model=CGB0"}, Settings{.allow_case_insensitivity = false});
        REQUIRE(exact);
        CHECK(exact->model == Model::CGB0);
    }
}

TEST_CASE("settings: allow_kebab_casing") {
    SUBCASE("enabled: snake_case fields accept kebab-case") {
        const auto args = parse_from<KebabArgs>({"--dry-run=42"}, Settings{.allow_kebab_casing = true});
        REQUIRE(args);
        CHECK(args->dry_run == 42);
    }
    SUBCASE("disabled: only the snake_case spelling is known") {
        const auto args = parse_from<KebabArgs>({"--dry-run=42"}, Settings{.allow_kebab_casing = false});
        REQUIRE_FALSE(args);
        CHECK(args.errors()[0].kind == ErrorKind::UNKNOWN_OPTION);
        const auto exact = parse_from<KebabArgs>({"--dry_run=42"}, Settings{.allow_kebab_casing = false});
        REQUIRE(exact);
        CHECK(exact->dry_run == 42);
    }
}
