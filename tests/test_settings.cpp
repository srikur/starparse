#include "test_support.hpp"

using namespace StarParse;

namespace {
    // Models from StarGBC
    enum class Model : std::uint8_t {
        Auto,
        DMG0,
        DMGA,
        DMGB,
        DMGC,
        MGB,
        SGB,
        SGB2,
        CGB0,
        CGBA,
        CGBB,
        CGBC,
        CGBD,
        CGBE,
        AGB0,
        AGBA,
        AGBAE,
        AGBB,
        AGBBE,
    };

    struct ModelArgs {
        [[=Opt{'m', "Model"}]] Model model{Model::Auto};
    };

    struct KebabArgs {
        int dry_run;
    };
} // namespace

TEST_CASE("settings: allow_case_insensitivity") {
    SUBCASE("enabled: enum values match in any case") {
        const auto args = parse_from<ModelArgs, {.allow_case_insensitivity = true}>({"--model=cgb0"});
        REQUIRE(args);
        CHECK(args->model == Model::CGB0);
    }
    SUBCASE("disabled: enum values must match exactly") {
        const auto args = parse_from<ModelArgs, {.allow_case_insensitivity = false}>({"--model=cgb0"});
        REQUIRE_FALSE(args);
        CHECK(args.errors()[0].kind == ErrorKind::INVALID_VALUE);
        const auto exact = parse_from<ModelArgs, {.allow_case_insensitivity = false}>({"--model=CGB0"});
        REQUIRE(exact);
        CHECK(exact->model == Model::CGB0);
    }
}

TEST_CASE("settings: allow_kebab_casing") {
    SUBCASE("enabled: snake_case fields accept kebab-case") {
        const auto args = parse_from<KebabArgs, {.allow_kebab_casing = true}>({"--dry-run=42"});
        REQUIRE(args);
        CHECK(args->dry_run == 42);
    }
    SUBCASE("disabled: only the snake_case spelling is known") {
        const auto args = parse_from<KebabArgs, {.allow_kebab_casing = false}>({"--dry-run=42"});
        REQUIRE_FALSE(args);
        CHECK(args.errors()[0].kind == ErrorKind::UNKNOWN_OPTION);
        const auto exact = parse_from<KebabArgs, {.allow_kebab_casing = false}>({"--dry_run=42"});
        REQUIRE(exact);
        CHECK(exact->dry_run == 42);
    }
}
