#include "test_support.hpp"

#include <optional>
#include <string>
#include <vector>

using namespace StarParse;

namespace {
    enum class Mode {
        MODE_0, MODE_1, MODE_2, MODE_3 [[=Alias{"m3", "3", "mode3"}]]
    };

    struct [[=Program{"starparse", "CMD line arg parser", "0.0.1"}]] Args {
        [[=Positional{0}]] std::string arg1;
        [[=Opt{'v', "Verbose mode"}]] bool verbose;
        [[=Opt{'k', "Flag k"}]] bool kay;
        [[=Opt{'f', "Flag f"}]] bool eff;
        [[=Opt{'o'}]] std::optional<std::string> output_file;
        [[=Opt{'m'}]] std::optional<Mode> mode;
        [[=Opt{'I'}]] std::vector<std::string> include_paths;
    };
}

TEST_CASE("parse: no arguments leaves every field at its default") {
    const auto args = parse_from<Args>({});
    REQUIRE(args);
    CHECK(args->arg1.empty());
    CHECK_FALSE(args->kay);
    CHECK_FALSE(args->verbose);
    CHECK_FALSE(args->eff);
    CHECK_FALSE(args->output_file.has_value());
    CHECK_FALSE(args->mode.has_value());
    CHECK(args->include_paths.empty());
}

TEST_CASE("parse: -kvf sets every flag in the bundle") {
    const auto args = parse_from<Args>({"-kvf"});
    REQUIRE(args);
    CHECK(args->kay);
    CHECK(args->verbose);
    CHECK(args->eff);
}

TEST_CASE("parse: --verbose sets only the named flag") {
    const auto args = parse_from<Args>({"--verbose"});
    REQUIRE(args);
    CHECK_FALSE(args->kay);
    CHECK(args->verbose);
    CHECK_FALSE(args->eff);
}

TEST_CASE("parse: separate short flags") {
    const auto args = parse_from<Args>({"-k", "-f"});
    REQUIRE(args);
    CHECK(args->kay);
    CHECK_FALSE(args->verbose);
    CHECK(args->eff);
}

TEST_CASE("parse: enum option by enumerator name") {
    const auto args = parse_from<Args>({"-m", "MODE_2"});
    REQUIRE(args);
    CHECK(args->mode == Mode::MODE_2);
}

TEST_CASE("parse: value attached to a short option") {
    const auto args = parse_from<Args>({"-ofilename"});
    REQUIRE(args);
    CHECK(args->output_file == "filename");
}

TEST_CASE("parse: repeated option appends to a vector") {
    const auto args = parse_from<Args>({"-I", "path1", "-I", "path2"});
    REQUIRE(args);
    const std::vector<std::string> expected{"path1", "path2"};
    CHECK(args->include_paths == expected);
}
