#include "test_support.hpp"

#include <optional>

namespace {
    using namespace StarParse;
    using namespace StarParse::detail::Assertions;

    struct Command {
        int value{};
    };

    struct MixedArgs {
        [[=Positional{0}]] int initial{};
        [[=Opt{'v'}, =Alias{"run"}]] bool verbose{};
        [[=Subcommand, =Alias{"r"}]] std::optional<Command> run;
    };

    static_assert(check_annotation_placement<MixedArgs>());
    static_assert(no_annotations_on_ignored_fields<MixedArgs>());
    static_assert(no_duplicate_annotations<MixedArgs>());
    static_assert(check_alias_collisions<MixedArgs>());
    static_assert(check_subcommand_collisions<MixedArgs>());

    struct DuplicateAnnotation {
        [[=Subcommand, =Subcommand]] std::optional<Command> run;
    };

    static_assert(!no_duplicate_annotations<DuplicateAnnotation>());

    struct [[=Subcommand]] MisplacedType {};

    static_assert(!check_annotation_placement<MisplacedType>());

    struct MisplacedStaticMember {
        [[=Subcommand]] inline static std::optional<Command> run;
    };

    static_assert(!check_annotation_placement<MisplacedStaticMember>());

    enum class MisplacedEnumerator {
        run [[=Subcommand]]
    };

    struct EnumArgs {
        MisplacedEnumerator mode{};
    };

    static_assert(!check_annotation_placement<EnumArgs>());

    struct CaseCollision {
        [[=Subcommand]] std::optional<Command> run;
        [[=Subcommand]] std::optional<Command> RUN;
    };

    static_assert(!check_subcommand_collisions<CaseCollision>());

    struct AliasCollision {
        [[=Subcommand, =Alias{"go"}]] std::optional<Command> run;
        [[=Subcommand, =Alias{"GO"}]] std::optional<Command> execute;
    };

    static_assert(!check_subcommand_collisions<AliasCollision>());

    struct AliasNameCollision {
        [[=Subcommand, =Alias{"execute"}]] std::optional<Command> run;
        [[=Subcommand]] std::optional<Command> execute;
    };

    static_assert(!check_subcommand_collisions<AliasNameCollision>());

    struct KebabCollision {
        [[=Subcommand]] std::optional<Command> dry_run;
        [[=Subcommand, =Alias{"dry-run"}]] std::optional<Command> execute;
    };

    static_assert(!check_subcommand_collisions<KebabCollision>());

    struct BuiltinNames {
        [[=Subcommand]] std::optional<Command> help;
        [[=Subcommand]] std::optional<Command> version;
    };

    static_assert(check_alias_collisions<BuiltinNames>());
    static_assert(check_subcommand_collisions<BuiltinNames>());

    struct NestedArgs {
        [[=Subcommand]] std::optional<MixedArgs> run;
    };

    static_assert(check_subcommand_collisions<NestedArgs>());
}

TEST_CASE("subcommand assertions: valid layouts pass every check") {
    StarParse::detail::Parser::check_assertions<MixedArgs>();
    StarParse::detail::Parser::check_assertions<BuiltinNames>();
    StarParse::detail::Parser::check_assertions<NestedArgs>();
    CHECK(check_subcommand_collisions<MixedArgs>());
    CHECK(check_subcommand_collisions<BuiltinNames>());
    CHECK(check_subcommand_collisions<NestedArgs>());
    CHECK_FALSE(check_subcommand_collisions<CaseCollision>());
    CHECK_FALSE(check_subcommand_collisions<KebabCollision>());
}
