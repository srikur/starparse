#include "test_support.hpp"

#include <filesystem>
#include <fstream>
#include <random>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

using namespace StarParse;

namespace {
    struct ResponseFile {
        std::filesystem::path directory = std::filesystem::temp_directory_path() /
                                          ("starparse-response-" + std::to_string(std::random_device{}()));
        std::filesystem::path path = directory / "arguments with spaces.rsp";
        std::string argument = "@" + path.string();

        explicit ResponseFile(const std::string_view contents) {
            REQUIRE(std::filesystem::create_directory(directory));
            std::ofstream file{path, std::ios::binary};
            file << contents;
            file.close();
            REQUIRE(file.good());
        }

        ~ResponseFile() {
            std::error_code error;
            std::filesystem::remove_all(directory, error);
        }
    };

    struct Args {
        [[=Opt{'j'}]] int jobs;
        [[=Opt{'v'}]] bool verbose;
        [[=Positional{0}]] std::string input;
    };

    static_assert(!std::is_copy_constructible_v<ParsedArgs<Args> >);
    static_assert(!std::is_copy_assignable_v<ParsedArgs<Args> >);
    static_assert(std::is_move_constructible_v<ParsedArgs<Args> >);
    static_assert(std::is_move_assignable_v<ParsedArgs<Args> >);
} // namespace

TEST_CASE("response files: split whitespace and skip a UTF-8 BOM") {
    const ResponseFile file{"\xEF\xBB\xBF  --jobs\t4\r\n--verbose\vinput.txt\f"};
    const auto result = detail::File::read_response_file(file.path.string());
    REQUIRE(result.has_value());
    CHECK(*result == std::vector<std::string>{"--jobs", "4", "--verbose", "input.txt"});
}

TEST_CASE("response files: quotes, escapes and empty arguments") {
    const ResponseFile file{
        R"(--input="hello world" 'single quoted' "" '' unquoted\ value "say \"hello\"" C:\\tmp pre"middle"'post' '#literal' $HOME)"
    };
    const auto result = detail::File::read_response_file(file.path.string());
    REQUIRE(result.has_value());
    CHECK(*result == std::vector<std::string>{"--input=hello world", "single quoted", "", "", "unquoted value",
          "say \"hello\"", "C:\\tmp", "premiddlepost", "#literal", "$HOME"});
}

TEST_CASE("response files: empty and whitespace-only files have no arguments") {
    for (const auto contents : {"", " \t\r\n\v\f"}) {
        const ResponseFile file{contents};
        const auto result = detail::File::read_response_file(file.path.string());
        REQUIRE(result.has_value());
        CHECK(result->empty());
        const auto args = parse_from<Args>({file.argument}, Args{.jobs = 7, .verbose = true, .input = "default"});
        REQUIRE(args);
        CHECK(args->jobs == 7);
        CHECK(args->verbose);
        CHECK(args->input == "default");
    }
}

TEST_CASE("response files: malformed quoting and escapes report errors") {
    for (const auto contents : {"--input 'unfinished", "--input \"unfinished", "--input unfinished\\"}) {
        const ResponseFile file{contents};
        const auto result = detail::File::read_response_file(file.path.string());
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error().kind == ErrorKind::INVALID_RESPONSE_FILE);
        CHECK(result.error().input_value == file.path.string());
        CHECK(result.error().detail == (std::string_view{contents}.ends_with('\\') ? "trailing escape" : "unterminated quote"));

        const auto args = parse_from<Args>({"--verbose", file.argument});
        REQUIRE_FALSE(args);
        REQUIRE(args.errors().size() == 1uz);
        CHECK(args.errors()[0].kind == ErrorKind::INVALID_RESPONSE_FILE);
        CHECK(args.errors()[0].argv_index == 2uz);
        CHECK(args.error_message() == result.error().to_string());
    }
}

TEST_CASE("response files: missing files report errors through parse and parse_or_throw") {
    const ResponseFile file{""};
    const auto missing = (file.directory / "missing.rsp").string();
    const auto result = detail::File::read_response_file(missing);
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().kind == ErrorKind::READING_RESPONSE_FAILED);
    CHECK(result.error().input_value == missing);
    const auto argument = "@" + missing;
    const char *argv[]{"program", argument.c_str()};
    const auto args = parse<Args>(2, argv);
    REQUIRE_FALSE(args);
    REQUIRE(args.errors().size() == 1uz);
    CHECK(args.errors()[0].argv_index == 1uz);
    CHECK(args.error_message() == "Failed to read response file '" + missing + "': could not open file");
    CHECK_THROWS_WITH_AS(parse_or_throw<Args>(2, argv), args.error_message().c_str(), std::invalid_argument);
    CHECK_FALSE(parse_from<Args>({"@"}));
}

TEST_CASE("response files: an unreadable directory reports a read error") {
    const ResponseFile file{""};
    const auto result = detail::File::read_response_file(file.directory.string());
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().kind == ErrorKind::READING_RESPONSE_FAILED);
}

TEST_CASE("response files: the first file supplies the argument list") {
    const ResponseFile first{"--jobs=4 --verbose \"hello world\""};
    const ResponseFile second{"--jobs=9"};
    const auto args = parse_from<Args>({"--jobs=2", first.argument, second.argument, "--jobs=6"});
    REQUIRE(args);
    CHECK(args->jobs == 4);
    CHECK(args->verbose);
    CHECK(args->input == "hello world");
}

TEST_CASE("response files: the option separator preserves literal at-prefixed arguments") {
    const auto literal = parse_from<Args>({"--", "@literal"});
    REQUIRE(literal);
    CHECK(literal->input == "@literal");

    const ResponseFile file{"-- @literal"};
    const auto args = parse_from<Args>({file.argument});
    REQUIRE(args);
    CHECK(args->input == "@literal");
}

TEST_CASE("response files: string views survive move construction and assignment") {
    struct Views {
        std::string_view short_value;
        std::string_view long_value;
    };
    auto args = [] {
        const ResponseFile previous{"--short-value=old --long-value='previous response file storage'"};
        auto result = parse_from<Views>({previous.argument});
        {
            const ResponseFile file{"--short-value=abc --long-value='a string longer than the small string optimization buffer'"};
            auto original = parse_from<Views>({file.argument});
            auto moved = std::move(original);
            result = std::move(moved);
        }
        return result;
    }();
    REQUIRE(args);
    CHECK(args->short_value == "abc");
    CHECK(args->long_value == "a string longer than the small string optimization buffer");
    const auto moved = std::move(args);
    CHECK(moved->short_value == "abc");
    CHECK(moved->long_value == "a string longer than the small string optimization buffer");
}

TEST_CASE("response files: error argument names survive moving the result") {
    const auto args = [] {
        const ResponseFile file{"--verbose --verbose"};
        auto original = parse_from<Args>({file.argument}, Settings{.duplicate_option_policy = DuplicateOptionPolicy::ERROR});
        return ParsedArgs{std::move(original)};
    }();
    REQUIRE_FALSE(args);
    REQUIRE(args.errors().size() == 1uz);
    CHECK(args.errors()[0].kind == ErrorKind::DUPLICATE_OPTION);
    CHECK(args.error_message() == "Duplicate option 'verbose' provided");
}

TEST_CASE("response files: subcommands and help are parsed normally") {
    struct Commands {
        [[=Subcommand]] std::optional<Args> build;
    };
    const ResponseFile file{"build --jobs=8 --help"};
    const auto args = parse_from<Commands>({file.argument});
    REQUIRE(args);
    REQUIRE(args->build.has_value());
    CHECK(args->build->jobs == 8);
    CHECK(args.help_requested());
    CHECK(args.help().find("build") != std::string::npos);
}
