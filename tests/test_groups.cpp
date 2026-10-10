#include "test_support.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

using namespace StarParse;

namespace {
    namespace Assertions = StarParse::detail::Assertions;

    struct NeedsFields {
        [[=Opt{'a'}, =Needs{"first", "second"}]] bool action{};
        [[=Opt{'f'}]] bool first{};
        [[=Opt{'s'}]] bool second{};
    };

    struct NeedsGroup {
        [[=Opt{'a'}, =Needs{"targets"}]] bool action{};
        [[=Opt{'f'}, =Groups{"targets"}]] bool first{};
        [[=Opt{'s'}, =Groups{"targets"}]] bool second{};
    };

    struct ExcludesFields {
        [[=Opt{'a'}, =Excludes{"first", "second"}]] bool action{};
        [[=Opt{'f'}]] bool first{};
        [[=Opt{'s'}]] bool second{};
    };

    struct ExcludesGroup {
        [[=Opt{'a'}, =Excludes{"targets"}]] bool action{};
        [[=Opt{'f'}, =Groups{"targets"}]] bool first{};
        [[=Opt{'s'}, =Groups{"targets"}]] bool second{};
    };

    template<typename T>
    auto parse_presence(const unsigned mask) {
        std::array<const char *, 4> argv{"groups-test"};
        constexpr std::array options{"--action", "--first", "--second"};
        int argc{1};
        for (const auto &[index, value] : std::views::enumerate(options)) {
            if (mask & (1u << index)) argv[argc++] = value;
        }
        return parse<T>(argc, argv.data());
    }

    // Compare error contents without depending on the order of group members.
    template<typename T>
    void check_errors(const ParsedArgs<T> &args, const ErrorKind kind, const std::vector<std::string_view> &names,
                      const std::optional<std::string_view> source = std::nullopt) {
        CAPTURE(args.error_message());
        CHECK(static_cast<bool>(args) == names.empty());
        CHECK(args.errors().size() == names.size());
        for (const auto &error : args.errors()) {
            CHECK(error.kind == kind);
            if (source)
                CHECK(error.current_argument == source);
        }
        for (const auto name : names) {
            CAPTURE(name);
            CHECK(std::ranges::count(args.errors(), name, &ParseError::detail) == 1);
        }
    }
} // namespace

TEST_CASE_TEMPLATE("groups: Needs checks every presence combination", T, NeedsFields, NeedsGroup) {
    for (auto mask : std::views::iota(0, 8)) {
        CAPTURE(mask);
        const auto args = parse_presence<T>(mask);
        std::vector<std::string_view> missing;
        if (mask & 1u) {
            if (!(mask & 2u)) missing.emplace_back("first");
            if (!(mask & 4u)) missing.emplace_back("second");
        }
        check_errors(args, ErrorKind::MISSING_DEPENDENCY, missing);
        if (args) {
            CHECK(args->action == static_cast<bool>(mask & 1u));
            CHECK(args->first == static_cast<bool>(mask & 2u));
            CHECK(args->second == static_cast<bool>(mask & 4u));
        }
    }
}

TEST_CASE_TEMPLATE("groups: Excludes checks every presence combination", T, ExcludesFields, ExcludesGroup) {
    for (auto mask : std::views::iota(0, 8)) {
        CAPTURE(mask);
        const auto args = parse_presence<T>(mask);
        std::vector<std::string_view> conflicts;
        if (mask & 1u) {
            if (mask & 2u) conflicts.emplace_back("first");
            if (mask & 4u) conflicts.emplace_back("second");
        }
        check_errors(args, ErrorKind::INVALID_OVERLAP, conflicts, "action");
    }
}

TEST_CASE_TEMPLATE("groups: explicitly false options still satisfy Needs", T, NeedsFields, NeedsGroup) {
    const auto args = parse_from<T>({"--action=false", "--no-first", "--second=false"});
    REQUIRE(args);
    CHECK_FALSE(args->action);
    CHECK_FALSE(args->first);
    CHECK_FALSE(args->second);

    check_errors(parse_from<T>({"--no-action"}), ErrorKind::MISSING_DEPENDENCY, {"first", "second"});
}

TEST_CASE_TEMPLATE("groups: explicitly false options still trigger Excludes", T, ExcludesFields, ExcludesGroup) {
    check_errors(parse_from<T>({"--action=false", "--no-first", "--second=false"}),
                 ErrorKind::INVALID_OVERLAP, {"first", "second"}, "action");
}

TEST_CASE_TEMPLATE("groups: prefilled values do not satisfy Needs", T, NeedsFields, NeedsGroup) {
    const T initial{.first = true, .second = true};
    check_errors(parse_from<T>({"--action"}, initial), ErrorKind::MISSING_DEPENDENCY, {"first", "second"});

    const auto args = parse_from<T>({"--action", "--no-first", "--no-second"}, initial);
    REQUIRE(args);
    CHECK_FALSE(args->first);
    CHECK_FALSE(args->second);
}

TEST_CASE_TEMPLATE("groups: prefilled values do not trigger Excludes", T, ExcludesFields, ExcludesGroup) {
    const auto args = parse_from<T>({"--action"}, T{.first = true, .second = true});
    REQUIRE(args);
    CHECK(args->first);
    CHECK(args->second);
}

TEST_CASE_TEMPLATE("groups: a prefilled owner does not activate its rules", T, NeedsFields, NeedsGroup, ExcludesFields, ExcludesGroup) {
    const auto args = parse_from<T>({}, T{.action = true});
    REQUIRE(args);
    CHECK(args->action);
}

TEST_CASE_TEMPLATE("groups: Needs is independent of argument order and supports short options", T, NeedsFields, NeedsGroup) {
    REQUIRE(parse_from<T>({"--second", "--first", "--action"}));
    REQUIRE(parse_from<T>({"-afs"}));
}

TEST_CASE_TEMPLATE("groups: Excludes is independent of argument order", T, ExcludesFields, ExcludesGroup) {
    check_errors(parse_from<T>({"--second", "--first", "--action"}),
                 ErrorKind::INVALID_OVERLAP, {"first", "second"}, "action");
}

TEST_CASE("groups: membership alone imposes no requirements and is not an option") {
    struct GroupMembership {
        [[=Opt{""}, =Groups{"targets"}]] bool first{};
        [[=Opt{""}, =Groups{"targets"}]] bool second{};
    };
    REQUIRE(parse_from<GroupMembership>({}));
    REQUIRE(parse_from<GroupMembership>({"--first"}));
    REQUIRE(parse_from<GroupMembership>({"--first", "--second"}));
    const auto args = parse_from<GroupMembership>({"--targets"});
    REQUIRE_FALSE(args);
    REQUIRE(args.errors().size() == 1);
    CHECK(args.errors()[0].kind == ErrorKind::UNKNOWN_OPTION);
}

TEST_CASE("groups: an option can belong to multiple independent groups") {
    struct MultipleMembership {
        [[=Opt{""}, =Needs{"primary"}]] bool action{};
        [[=Opt{""}, =Groups{"primary", "secondary"}]] bool shared{};
        [[=Opt{""}, =Groups{"secondary"}]] bool other{};
    };
    REQUIRE(parse_from<MultipleMembership>({"--action", "--shared"}));
    check_errors(parse_from<MultipleMembership>({"--action", "--other"}), ErrorKind::MISSING_DEPENDENCY, {"shared"});
}

TEST_CASE("groups: Needs can mix a field and multiple groups") {
    struct MixedNeeds {
        [[=Opt{""}, =Needs{"token", "input", "output"}]] bool action{};
        [[=Opt{""}]] bool token{};
        [[=Opt{""}, =Groups{"input"}]] bool first{};
        [[=Opt{""}, =Groups{"output"}]] bool second{};
    };
    REQUIRE(parse_from<MixedNeeds>({"--action", "--token", "--first", "--second"}));
    check_errors(parse_from<MixedNeeds>({"--action", "--first"}), ErrorKind::MISSING_DEPENDENCY, {"token", "second"});
}

TEST_CASE("groups: Excludes can mix a field and multiple groups") {
    struct MixedExcludes {
        [[=Opt{""}, =Excludes{"token", "input", "output"}]] bool action{};
        [[=Opt{""}]] bool token{};
        [[=Opt{""}, =Groups{"input"}]] bool first{};
        [[=Opt{""}, =Groups{"output"}]] bool second{};
    };
    REQUIRE(parse_from<MixedExcludes>({"--action"}));
    check_errors(parse_from<MixedExcludes>({"--action", "--token", "--first", "--second"}),
                 ErrorKind::INVALID_OVERLAP, {"token", "first", "second"}, "action");
}

TEST_CASE("groups: Needs and Excludes on the same option both run") {
    struct CombinedRules {
        [[=Opt{""}, =Needs{"required"}, =Excludes{"forbidden"}]] bool action{};
        [[=Opt{""}, =Groups{"required"}]] bool first{};
        [[=Opt{""}, =Groups{"forbidden"}]] bool second{};
    };
    REQUIRE(parse_from<CombinedRules>({"--action", "--first"}));
    const auto args = parse_from<CombinedRules>({"--action", "--second"});
    REQUIRE_FALSE(args);
    REQUIRE(args.errors().size() == 2);
    CHECK(args.errors()[0].kind == ErrorKind::MISSING_DEPENDENCY);
    CHECK(args.errors()[0].detail == "first");
    CHECK(args.errors()[1].kind == ErrorKind::INVALID_OVERLAP);
    CHECK(args.errors()[1].detail == "second");
    CHECK(args.errors()[1].current_argument == "action");
}

TEST_CASE("groups: renamed members and aliases satisfy a group dependency") {
    struct RenamedGroup {
        [[=Opt{'a'}, =Name{"launch"}, =Alias{"start"}, =Needs{"settings"}]] bool action{};
        [[=Opt{'d'}, =Name{"dry_run"}, =Alias{"preview"}, =Groups{"settings"}]] bool cpp_flag{};
    };
    for (const auto option : {"--dry_run", "--dry-run", "--DRY-RUN", "--preview", "-d"}) {
        CAPTURE(option);
        const auto args = parse_from<RenamedGroup>({"--start", option});
        REQUIRE(args);
        CHECK(args->cpp_flag);
    }
    check_errors(parse_from<RenamedGroup>({"--launch"}), ErrorKind::MISSING_DEPENDENCY, {"dry_run"});
}

TEST_CASE("groups: field dependencies use resolved names and accept aliases on the command line") {
    struct RenamedDependency {
        [[=Opt{""}, =Needs{"target"}]] bool action{};
        [[=Opt{""}, =Name{"target"}, =Alias{"alias"}]] bool cpp_target{};
    };
    REQUIRE(parse_from<RenamedDependency>({"--action", "--alias"}));
    check_errors(parse_from<RenamedDependency>({"--action"}), ErrorKind::MISSING_DEPENDENCY, {"target"});
}

TEST_CASE("groups: exclusion diagnostics use both resolved names") {
    struct RenamedExclusion {
        [[=Opt{""}, =Name{"launch"}, =Excludes{"settings"}]] bool action{};
        [[=Opt{""}, =Name{"dry_run"}, =Alias{"preview"}, =Groups{"settings"}]] bool cpp_flag{};
    };
    const auto args = parse_from<RenamedExclusion>({"--launch", "--preview"});
    check_errors(args, ErrorKind::INVALID_OVERLAP, {"dry_run"}, "launch");
    CHECK(args.error_message() == "Option 'launch' cannot be used in conjunction with: 'dry_run'");
}

TEST_CASE("groups: required options and fixed arrays still enforce dependencies") {
    SUBCASE("required owner") {
        struct RequiredOwner {
            [[=Opt{""}, =Required, =Needs{"targets"}]] bool action{};
            [[=Opt{""}, =Groups{"targets"}]] bool target{};
        };
        check_errors(parse_from<RequiredOwner>({"--action"}), ErrorKind::MISSING_DEPENDENCY, {"target"});
        REQUIRE(parse_from<RequiredOwner>({"--action", "--target"}));
    }
    SUBCASE("array owner") {
        struct ArrayOwner {
            [[=Opt{""}, =Needs{"targets"}]] std::array<int, 2> values{};
            [[=Opt{""}, =Groups{"targets"}]] bool target{};
        };
        check_errors(parse_from<ArrayOwner>({"--values=1,2"}), ErrorKind::MISSING_DEPENDENCY, {"target"});
        REQUIRE(parse_from<ArrayOwner>({"--values=1,2", "--target"}));
    }
}

TEST_CASE("groups: positional optional and container members can satisfy a group") {
    struct InputGroup {
        [[=Opt{""}, =Needs{"inputs"}]] bool action{};
        [[=Positional{0}, =Groups{"inputs"}]] std::string input;
        [[=Opt{""}, =Groups{"inputs"}]] std::optional<int> count;
        [[=Opt{""}, =Groups{"inputs"}]] std::vector<int> values;
    };
    const auto args = parse_from<InputGroup>({"--action", "file.txt", "--count=0", "--values=0"});
    REQUIRE(args);
    CHECK(args->input == "file.txt");
    CHECK(args->count == 0);
    CHECK(args->values == std::vector{0});
    check_errors(parse_from<InputGroup>({"--action", "file.txt"}), ErrorKind::MISSING_DEPENDENCY, {"count", "values"});
}

TEST_CASE("groups: mutual dependencies allow neither or both options") {
    struct MutualNeeds {
        [[=Opt{""}, =Needs{"second"}]] bool first{};
        [[=Opt{""}, =Needs{"first"}]] bool second{};
    };
    SUBCASE("neither") { REQUIRE(parse_from<MutualNeeds>({})); }
    SUBCASE("both") { REQUIRE(parse_from<MutualNeeds>({"--first", "--second"})); }
    SUBCASE("one") { check_errors(parse_from<MutualNeeds>({"--first"}), ErrorKind::MISSING_DEPENDENCY, {"second"}); }
}

TEST_CASE("groups: mutual exclusions allow either option alone") {
    struct MutualExcludes {
        [[=Opt{""}, =Excludes{"second"}]] bool first{};
        [[=Opt{""}, =Excludes{"first"}]] bool second{};
    };
    SUBCASE("neither") { REQUIRE(parse_from<MutualExcludes>({})); }
    SUBCASE("first") { REQUIRE(parse_from<MutualExcludes>({"--first"})); }
    SUBCASE("second") { REQUIRE(parse_from<MutualExcludes>({"--second"})); }
    SUBCASE("both") {
        const auto args = parse_from<MutualExcludes>({"--first", "--second"});
        check_errors(args, ErrorKind::INVALID_OVERLAP, {"first", "second"});
        for (const auto &error : args.errors())
            CHECK(error.current_argument == (error.detail == "first" ? "second" : "first"));
    }
}

TEST_CASE("groups: a dependency chain validates supplied owners") {
    struct DependencyChain {
        [[=Opt{""}, =Needs{"second"}]] bool first{};
        [[=Opt{""}, =Needs{"third"}]] bool second{};
        [[=Opt{""}]] bool third{};
    };
    REQUIRE(parse_from<DependencyChain>({"--first", "--second", "--third"}));
    check_errors(parse_from<DependencyChain>({"--first", "--second"}), ErrorKind::MISSING_DEPENDENCY, {"third"});
    check_errors(parse_from<DependencyChain>({"--first"}), ErrorKind::MISSING_DEPENDENCY, {"second"});
}

TEST_CASE_TEMPLATE("groups: help and version bypass dependency validation", T, NeedsGroup, ExcludesGroup) {
    const auto help = parse_from<T>({"--action", "--first", "--help"});
    REQUIRE(help);
    CHECK(help.help_requested());
    const auto version = parse_from<T>({"--action", "--first", "--version"});
    REQUIRE(version);
    CHECK(version.version_requested());
}

TEST_CASE("groups: dependencies are checked within the selected subcommand") {
    struct Commands {
        [[=Subcommand]] std::optional<NeedsGroup> run;
        [[=Subcommand]] std::optional<ExcludesGroup> other;
    };
    REQUIRE(parse_from<Commands>({}));
    const auto complete = parse_from<Commands>({"run", "--action", "--first", "--second"});
    REQUIRE(complete);
    REQUIRE(complete->run.has_value());
    CHECK_FALSE(complete->other.has_value());
    check_errors(parse_from<Commands>({"run", "--action", "--first"}), ErrorKind::MISSING_DEPENDENCY, {"second"});
    check_errors(parse_from<Commands>({"other", "--action", "--second"}), ErrorKind::INVALID_OVERLAP, {"second"}, "action");
}

TEST_CASE("groups: compile-time checks accept empty and valid layouts") {
    struct Empty {};
    struct Plain {
        bool flag{};
    };
    CHECK(Assertions::check_exclusion_groups<Empty>());
    CHECK(Assertions::check_exclusion_groups<Plain>());
    CHECK(Assertions::check_group_name_collisions<Empty>());
    CHECK(Assertions::check_group_name_collisions<Plain>());
    CHECK(Assertions::check_exclusion_groups<NeedsFields>());
    CHECK(Assertions::check_exclusion_groups<NeedsGroup>());
    CHECK(Assertions::check_exclusion_groups<ExcludesFields>());
    CHECK(Assertions::check_exclusion_groups<ExcludesGroup>());
    CHECK(Assertions::check_group_name_collisions<NeedsGroup>());
    CHECK(Assertions::check_group_name_collisions<ExcludesGroup>());
}

TEST_CASE("groups: compile-time checks reject unknown and self references") {
    struct UnknownNeed {
        [[=Needs{"target", "missing"}]] bool action{};
        bool target{};
    };
    struct UnknownExclusion {
        [[=Excludes{"target", "missing"}]] bool action{};
        bool target{};
    };
    struct SelfNeed {
        [[=Needs{"action"}]] bool action{};
    };
    struct SelfExclusion {
        [[=Excludes{"action"}]] bool action{};
    };
    struct RenamedSelf {
        [[=Name{"launch"}, =Needs{"launch"}]] bool action{};
    };
    CHECK_FALSE(Assertions::check_exclusion_groups<UnknownNeed>());
    CHECK_FALSE(Assertions::check_exclusion_groups<UnknownExclusion>());
    CHECK_FALSE(Assertions::check_exclusion_groups<SelfNeed>());
    CHECK_FALSE(Assertions::check_exclusion_groups<SelfExclusion>());
    CHECK_FALSE(Assertions::check_exclusion_groups<RenamedSelf>());
}

TEST_CASE("groups: reference validation uses exact resolved names") {
    struct Resolved {
        [[=Needs{"target"}]] bool action{};
        [[=Name{"target"}, =Alias{"alias"}]] bool cpp_target{};
    };
    struct OldIdentifier {
        [[=Needs{"cpp_target"}]] bool action{};
        [[=Name{"target"}]] bool cpp_target{};
    };
    struct AliasReference {
        [[=Excludes{"alias"}]] bool action{};
        [[=Alias{"alias"}]] bool target{};
    };
    struct ShortReference {
        [[=Opt{""}, =Needs{"t"}]] bool action{};
        [[=Opt{'t'}]] bool target{};
    };
    struct WrongCase {
        [[=Needs{"TARGETS"}]] bool action{};
        [[=Groups{"targets"}]] bool target{};
    };
    CHECK(Assertions::check_exclusion_groups<Resolved>());
    CHECK_FALSE(Assertions::check_exclusion_groups<OldIdentifier>());
    CHECK_FALSE(Assertions::check_exclusion_groups<AliasReference>());
    CHECK_FALSE(Assertions::check_exclusion_groups<ShortReference>());
    CHECK_FALSE(Assertions::check_exclusion_groups<WrongCase>());
}

TEST_CASE("groups: group names cannot collide with resolved field names") {
    struct OwnName {
        [[=Groups{"action"}]] bool action{};
    };
    struct OtherName {
        [[=Groups{"target"}]] bool action{};
        bool target{};
    };
    struct Renamed {
        [[=Groups{"target"}]] bool action{};
        [[=Name{"target"}]] bool cpp_target{};
    };
    struct OldNameAvailable {
        [[=Groups{"cpp_target"}]] bool action{};
        [[=Name{"target"}]] bool cpp_target{};
    };
    CHECK_FALSE(Assertions::check_group_name_collisions<OwnName>());
    CHECK_FALSE(Assertions::check_group_name_collisions<OtherName>());
    CHECK_FALSE(Assertions::check_group_name_collisions<Renamed>());
    CHECK(Assertions::check_group_name_collisions<OldNameAvailable>());
}

TEST_CASE("groups: parent references cannot resolve groups inside a child") {
    struct Commands {
        [[=Opt{""}, =Needs{"targets"}]] bool action{};
        [[=Subcommand]] std::optional<NeedsGroup> run;
    };
    CHECK_FALSE(Assertions::check_exclusion_groups<Commands>());
}
