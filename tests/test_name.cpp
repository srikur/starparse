#include "test_support.hpp"

#include <array>
#include <expected>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

using namespace StarParse;

namespace {
    struct Args {
        [[=Opt{'j'}, =Name{"thread_count"}, =Alias{"workers"}]] int jobs{};
        [[=Opt{'q'}, =Name{"chatty_mode"}, =Alias{"talkative"}]] bool verbose{};
    };

    struct BareArgs {
        [[=Name{"source-file"}]] std::string cpp_source;
    };

    struct PositionalArgs {
        [[=Positional{0}, =Required, =Name{"source-file"}]] std::string cpp_source;
    };

    struct Command {
        [[=Opt{'n'}, =Name{"amount"}]] int cpp_value{};
    };

    struct Commands {
        [[=Subcommand, =Name{"dry_run"}, =Alias{"preview"}]] std::optional<Command> cpp_command;
    };
}

TEST_CASE("name: override replaces the identifier in every option spelling") {
    for (const auto option : {"--thread_count=4", "-thread_count=4", "--thread-count=4", "--THREAD-COUNT=4"}) {
        CAPTURE(option);
        const auto args = parse_from<Args>({option});
        REQUIRE(args);
        CHECK(args->jobs == 4);
    }
    for (const auto option : {"--thread_count", "-thread_count", "--thread-count"}) {
        CAPTURE(option);
        const auto args = parse_from<Args>({option, "4"});
        REQUIRE(args);
        CHECK(args->jobs == 4);
    }
    for (const auto option : {"--jobs=4", "-jobs=4", "--JOBS=4", "--verbose", "--no-verbose"}) {
        CAPTURE(option);
        const auto args = parse_from<Args>({option});
        REQUIRE_FALSE(args);
        REQUIRE(args.errors().size() == 1);
        CHECK(args.errors()[0].kind == ErrorKind::UNKNOWN_OPTION);
    }
}

TEST_CASE("name: explicit aliases and short options remain available") {
    const auto aliases = parse_from<Args>({"--workers=4", "--talkative"});
    REQUIRE(aliases);
    CHECK(aliases->jobs == 4);
    CHECK(aliases->verbose);

    const auto bundle = parse_from<Args>({"-qj4"});
    REQUIRE(bundle);
    CHECK(bundle->jobs == 4);
    CHECK(bundle->verbose);
}

TEST_CASE("name: settings apply to the override") {
    constexpr Settings strict{.allow_kebab_casing = false, .allow_aliases = false, .allow_case_insensitivity = false};
    const auto exact = parse_from<Args>({"--thread_count=4"}, strict);
    REQUIRE(exact);
    CHECK(exact->jobs == 4);
    for (const auto option : {"--thread-count=4", "--THREAD_COUNT=4", "--workers=4", "--jobs=4"}) {
        CAPTURE(option);
        const auto args = parse_from<Args>({option}, strict);
        REQUIRE_FALSE(args);
        REQUIRE(args.errors().size() == 1);
        CHECK(args.errors()[0].kind == ErrorKind::UNKNOWN_OPTION);
    }
}

TEST_CASE("name: negations use the override") {
    for (const auto option : {"--no-chatty_mode", "--no-chatty-mode"}) {
        CAPTURE(option);
        const auto args = parse_from<Args>({option}, Args{.verbose = true});
        REQUIRE(args);
        CHECK_FALSE(args->verbose);
    }
    const auto disabled = parse_from<Args>({"--no-chatty-mode"}, Settings{.autogenerate_negations = false});
    REQUIRE_FALSE(disabled);
    CHECK(disabled.errors()[0].kind == ErrorKind::UNKNOWN_OPTION);
}

TEST_CASE("name: literal no-prefix is part of the override") {
    struct NoCache {
        [[=Name{"no-cache"}]] bool disable_cache{};
    };
    const auto enabled = parse_from<NoCache>({"--no-cache"});
    REQUIRE(enabled);
    CHECK(enabled->disable_cache);
    const auto negated = parse_from<NoCache>({"--no-no-cache"}, NoCache{true});
    REQUIRE(negated);
    CHECK_FALSE(negated->disable_cache);
}

TEST_CASE("name: bare and positional fields accept only the override as a name") {
    const auto bare = parse_from<BareArgs>({"--source-file=data.txt"});
    REQUIRE(bare);
    CHECK(bare->cpp_source == "data.txt");
    const auto bare_positional = parse_from<BareArgs>({"data.txt"});
    REQUIRE(bare_positional);
    CHECK(bare_positional->cpp_source == "data.txt");
    const auto named = parse_from<PositionalArgs>({"--source-file=data.txt"});
    REQUIRE(named);
    CHECK(named->cpp_source == "data.txt");
    const auto positional = parse_from<PositionalArgs>({"data.txt"});
    REQUIRE(positional);
    CHECK(positional->cpp_source == "data.txt");
    const auto old_name = parse_from<BareArgs>({"--cpp_source=data.txt"});
    REQUIRE_FALSE(old_name);
    CHECK(old_name.errors()[0].kind == ErrorKind::UNKNOWN_OPTION);
}

TEST_CASE("name: single-character overrides take precedence over builtin shortcuts") {
    struct Shortcuts {
        [[=Name{"h"}]] bool cpp_help{};
        [[=Name{"v"}]] bool cpp_version{};
    };
    const auto args = parse_from<Shortcuts>({"-H", "-v"});
    REQUIRE(args);
    CHECK(args->cpp_help);
    CHECK(args->cpp_version);
    CHECK_FALSE(args.help_requested());
    CHECK_FALSE(args.version_requested());
    const auto help = parse_from<Shortcuts>({"--help"});
    REQUIRE(help);
    CHECK(help.help_requested());
    CHECK(help.help().find("--help, -h") == std::string::npos);
    CHECK(help.help().find("--version, -v") == std::string::npos);
}

TEST_CASE("name: subcommands use the override and its aliases") {
    for (const auto command : {"dry_run", "dry-run", "DRY-RUN", "preview"}) {
        CAPTURE(command);
        const auto args = parse_from<Commands>({command, "--amount=4"});
        REQUIRE(args);
        REQUIRE(args->cpp_command.has_value());
        CHECK(args->cpp_command->cpp_value == 4);
    }
    const auto old_name = parse_from<Commands>({"cpp_command"});
    REQUIRE_FALSE(old_name);
    CHECK(old_name.errors()[0].kind == ErrorKind::UNKNOWN_OPTION);
}

TEST_CASE("name: help uses overrides for options positionals and command paths") {
    const auto options = parse_from<Args>({"--help"}).help();
    CHECK(options.find("--thread_count") != std::string::npos);
    CHECK(options.find("--chatty_mode") != std::string::npos);
    CHECK(options.find("--jobs") == std::string::npos);
    CHECK(options.find("--verbose") == std::string::npos);

    const auto positionals = parse_from<PositionalArgs>({"--help"}).help();
    CHECK(positionals.find("<source-file>") != std::string::npos);
    CHECK(positionals.find("--source-file") != std::string::npos);
    CHECK(positionals.find("cpp_source") == std::string::npos);

    const auto commands = parse_from<Commands>({"--help"}).help();
    CHECK(commands.find("dry_run, preview") != std::string::npos);
    CHECK(commands.find("cpp_command") == std::string::npos);

    const auto child = parse_from<Commands>({"preview", "--help"});
    REQUIRE(child);
    CHECK(child.help_requested());
    const auto child_help = child.help();
    CHECK(child_help.find("Usage: program [options] dry_run [options]") != std::string::npos);
    CHECK(child_help.find("--amount") != std::string::npos);
    CHECK(child_help.find("cpp_command") == std::string::npos);
    CHECK(child_help.find("cpp_value") == std::string::npos);
}

TEST_CASE("name: required and missing-value diagnostics use the override") {
    const auto required = parse_from<PositionalArgs>({});
    REQUIRE_FALSE(required);
    REQUIRE(required.errors().size() == 1);
    CHECK(required.errors()[0].kind == ErrorKind::MISSING_REQUIRED);
    CHECK(required.errors()[0].current_argument == "source-file");

    const auto missing = parse_from<Args>({"--thread_count"}, Settings{.allow_repeated_counts = false});
    REQUIRE_FALSE(missing);
    CHECK(missing.errors()[0].kind == ErrorKind::MISSING_VALUE);
    CHECK(missing.errors()[0].current_argument == "thread_count");
}

TEST_CASE("name: validation diagnostics use the override") {
    struct Validated {
        [[=Name{"minimum"}, =Min{2}]] int cpp_min{};
        [[=Name{"maximum"}, =Max{2}]] int cpp_max{};
        [[=Name{"interval"}, =Range{1, 2}]] int cpp_range{};
        [[=Name{"choice"}, =Choices{1, 2}]] int cpp_choice{};
        [[=Name{"positive"}, =Validator{[](const int &value) { return value > 0; }}]] int cpp_validator{};
    };
    const auto args = parse_from<Validated>({"--minimum=1", "--maximum=3", "--interval=3", "--choice=3", "--positive=-1"});
    REQUIRE_FALSE(args);
    constexpr std::array names{"minimum", "maximum", "interval", "choice", "positive"};
    REQUIRE(args.errors().size() == names.size());
    for (size_t i = 0; i < names.size(); ++i)
        CHECK(args.errors()[i].current_argument == names[i]);
}

TEST_CASE("name: count custom-parser and array diagnostics use the override") {
    SUBCASE("count overflow") {
        struct Count {
            [[=Name{"iterations"}]] unsigned cpp_count{std::numeric_limits<unsigned>::max()};
        };
        const auto args = parse_from<Count>({"--iterations"});
        REQUIRE_FALSE(args);
        CHECK(args.errors()[0].kind == ErrorKind::OUT_OF_RANGE);
        CHECK(args.errors()[0].current_argument == "iterations");
    }
    SUBCASE("custom parser") {
        struct Custom {
            [[=Name{"amount"}, =Parser<int>{
                [](const std::string_view &) -> std::expected<int, std::string> {
                    return std::unexpected{"invalid amount"};
                }
            }]] int cpp_value{};
        };
        const auto args = parse_from<Custom>({"--amount=bad"});
        REQUIRE_FALSE(args);
        CHECK(args.errors()[0].kind == ErrorKind::CUSTOM_PARSING_FAILED);
        CHECK(args.errors()[0].current_argument == "amount");
    }
    SUBCASE("array capacity") {
        struct Array {
            [[=Name{"values"}]] std::array<int, 2> cpp_values{};
        };
        const auto args = parse_from<Array>({"--values=1,2,3"});
        REQUIRE_FALSE(args);
        CHECK(args.errors()[0].kind == ErrorKind::DUPLICATE_OPTION);
        CHECK(args.errors()[0].current_argument == "values");
    }
}

TEST_CASE("name: the old identifier can be used by another field") {
    struct Reused {
        [[=Name{"threads"}]] int jobs{};
        [[=Name{"jobs"}]] int count{};
    };
    const auto args = parse_from<Reused>({"--threads=4", "--jobs=2"});
    REQUIRE(args);
    CHECK(args->jobs == 4);
    CHECK(args->count == 2);
}
