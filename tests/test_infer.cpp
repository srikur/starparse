#include "test_support.hpp"

#include <optional>
#include <string>

using namespace StarParse;

namespace {
    struct Args {
        [[=Opt{'V', "Verbose mode"}]] bool verbose{false};
        [[=Opt{'o', "Output file"}]] std::string output{};
        [[=Opt{'c', "Colorize"}, =Alias{"colour"}]] bool color{true};
        [[=Opt{'d', "Dry run level"}]] int dry_run{0};
        [[=Positional{0}]] std::string file{};
    };

    struct Overlap {
        [[=Opt{'o', "Short name"}]] std::string out;
        [[=Opt{'O', "Long name"}]] std::string output;
    };

    struct Build {
        [[=Positional{0}]] std::string target;
    };

    struct Bench {
        [[=Positional{0}]] int iterations;
    };

    struct Deploy {
        bool force{false};
    };

    struct Commands {
        bool verbose{false};
        [[=Subcommand]] std::optional<Build> build;
        [[=Subcommand]] std::optional<Bench> bench;
        [[=Subcommand, =Alias{"ship"}]] std::optional<Deploy> deploy;
    };

    constexpr Settings infer_subcommands{.infer_subcommands = true};
} // namespace

TEST_CASE("infer: unique prefix resolves to a flag") {
    for (const auto option : {"--verb", "--verbo", "--v", "--VERB"}) {
        CAPTURE(option);
        const auto args = parse_from<Args>({option, "in.txt"});
        REQUIRE(args);
        CHECK(args->verbose);
        CHECK(args->file == "in.txt");
    }
}

TEST_CASE("infer: unique prefix of an option that takes a value consumes the next argument") {
    const auto spaced = parse_from<Args>({"--out", "result.bin", "in.txt"});
    REQUIRE(spaced);
    CHECK(spaced->output == "result.bin");
    CHECK(spaced->file == "in.txt");
    const auto equals = parse_from<Args>({"--ou=result.bin", "in.txt"});
    REQUIRE(equals);
    CHECK(equals->output == "result.bin");
    CHECK(equals->file == "in.txt");
}

TEST_CASE("infer: kebab, alias, and negated spellings are inferable") {
    const auto kebab = parse_from<Args>({"--dry=3", "in.txt"});
    REQUIRE(kebab);
    CHECK(kebab->dry_run == 3);
    const auto alias = parse_from<Args>({"--colou", "in.txt"}, Args{.color = false});
    REQUIRE(alias);
    CHECK(alias->color);
    const auto negated = parse_from<Args>({"--no-col", "in.txt"});
    REQUIRE(negated);
    CHECK_FALSE(negated->color);
}

TEST_CASE("infer: builtin help and version are inferable") {
    const auto help = parse_from<Args>({"--he"});
    REQUIRE(help);
    CHECK(help.help_requested());
    const auto version = parse_from<Args>({"--vers"});
    REQUIRE(version);
    CHECK(version.version_requested());
}

TEST_CASE("infer: ambiguous prefix is reported with the candidates") {
    const auto args = parse_from<Args>({"--ver", "in.txt"});
    REQUIRE_FALSE(args);
    REQUIRE(args.errors().size() == 1uz);
    CHECK(args.errors()[0].kind == ErrorKind::AMBIGUOUS_OPTION);
    CHECK(args.errors()[0].argv_index == 1uz);
    CHECK(args.error_message() == "Ambiguous option 'ver'. Potential matches are --verbose, --version");
    const auto negations = parse_from<Args>({"--no", "in.txt"});
    REQUIRE_FALSE(negations);
    REQUIRE(negations.errors().size() == 1uz);
    CHECK(negations.errors()[0].kind == ErrorKind::AMBIGUOUS_OPTION);
}

TEST_CASE("infer: an exact spelling wins over a longer option it prefixes") {
    const auto args = parse_from<Overlap>({"--out=a", "--output=b"});
    REQUIRE(args);
    CHECK(args->out == "a");
    CHECK(args->output == "b");
    const auto longer = parse_from<Overlap>({"--outp=b"});
    REQUIRE(longer);
    CHECK(longer->out.empty());
    CHECK(longer->output == "b");
}

TEST_CASE("infer: disabled setting keeps abbreviations unknown") {
    constexpr Settings settings{.infer_arguments = false};
    const auto args = parse_from<Args, settings>({"--verb", "in.txt"});
    REQUIRE_FALSE(args);
    REQUIRE(args.errors().size() == 1uz);
    CHECK(args.errors()[0].kind == ErrorKind::UNKNOWN_OPTION);
    CHECK(args.error_message() == "Unknown option 'verb'");
    const auto close = parse_from<Args, settings>({"--verbos", "in.txt"});
    REQUIRE_FALSE(close);
    CHECK(close.error_message() == "Unknown option 'verbos'. Did you mean 'verbose'?");
    const auto ambiguous = parse_from<Args, settings>({"--ver", "in.txt"});
    REQUIRE_FALSE(ambiguous);
    CHECK(ambiguous.errors()[0].kind == ErrorKind::UNKNOWN_OPTION);
}

TEST_CASE("infer: case-insensitive prefixes follow the case setting") {
    const auto exact = parse_from<Args, Settings{.allow_case_insensitivity = false}>({"--VERB", "in.txt"});
    REQUIRE_FALSE(exact);
    CHECK(exact.errors()[0].kind == ErrorKind::UNKNOWN_OPTION);
}

TEST_CASE("infer: subcommand prefixes") {
    SUBCASE("unique prefix enters the subcommand") {
        const auto args = parse_from<Commands, infer_subcommands>({"--verbose", "bu", "all"});
        REQUIRE(args);
        CHECK(args->verbose);
        REQUIRE(args->build.has_value());
        CHECK(args->build->target == "all");
        CHECK_FALSE(args->bench.has_value());
    }
    SUBCASE("subcommand aliases are inferable") {
        const auto args = parse_from<Commands, infer_subcommands>({"sh", "--force"});
        REQUIRE(args);
        REQUIRE(args->deploy.has_value());
        CHECK(args->deploy->force);
    }
    SUBCASE("ambiguous prefix is not a subcommand") {
        const auto args = parse_from<Commands, infer_subcommands>({"b"});
        REQUIRE_FALSE(args);
        CHECK(args.errors()[0].kind == ErrorKind::UNKNOWN_OPTION);
        CHECK_FALSE(args->build.has_value());
        CHECK_FALSE(args->bench.has_value());
    }
    SUBCASE("disabled by default") {
        const auto args = parse_from<Commands>({"bu", "all"});
        REQUIRE_FALSE(args);
        CHECK(args.errors()[0].kind == ErrorKind::UNKNOWN_OPTION);
        CHECK_FALSE(args->build.has_value());
    }
}
