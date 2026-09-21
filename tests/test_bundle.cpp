#include "test_support.hpp"

#include <optional>
#include <string>
#include <vector>

using namespace StarParse;

namespace {
    struct Flags {
        [[=Positional{0}]] std::string arg1;
        [[=Opt{'k', "Keep going"}]] bool keep;
        [[=Opt{'v', "Verbose mode"}]] bool verbose;
        [[=Opt{'f', "Force"}]] bool force;
        [[=Positional{1}]] int arg2;
    };

    struct Jobs {
        [[=Opt{'k', "Keep going"}, =Alias{"q"}]] bool keep;
        [[=Opt{'j', "Num jobs"}, =Alias{"n"}]] int jobs;
    };

    struct Values {
        [[=Positional{0}]] std::string arg1;
        [[=Opt{'v', "Verbose mode"}]] bool verbose;
        [[=Opt{'k', "Flag k"}]] bool kay;
        [[=Opt{'o'}]] std::optional<std::string> output_file;
        [[=Opt{'I'}]] std::vector<std::string> include_paths;
    };
}

TEST_CASE("bundle: flags mixed with positionals") {
    const auto args = parse_from<Flags>({"-kvf", "hello", "42"});
    REQUIRE(args);
    CHECK(args->arg1 == "hello");
    CHECK(args->keep);
    CHECK(args->verbose);
    CHECK(args->force);
    CHECK(args->arg2 == 42);
}

TEST_CASE("bundle: --name=value form") {
    const auto args = parse_from<Flags>({"--arg1=hello", "--arg2=42"});
    REQUIRE(args);
    CHECK(args->arg1 == "hello");
    CHECK_FALSE(args->keep);
    CHECK_FALSE(args->verbose);
    CHECK_FALSE(args->force);
    CHECK(args->arg2 == 42);
}

TEST_CASE("bundle: value attached to the last option") {
    SUBCASE("-kj4") {
        const auto args = parse_from<Jobs>({"-kj4"});
        REQUIRE(args);
        CHECK(args->keep);
        CHECK(args->jobs == 4);
    }
    SUBCASE("-kj42") {
        const auto args = parse_from<Jobs>({"-kj42"});
        REQUIRE(args);
        CHECK(args->keep);
        CHECK(args->jobs == 42);
    }
    SUBCASE("-kj-4") {
        const auto args = parse_from<Jobs>({"-kj-4"});
        REQUIRE(args);
        CHECK(args->keep);
        CHECK(args->jobs == -4);
    }
    SUBCASE("-kj 4") {
        const auto args = parse_from<Jobs>({"-kj", "4"});
        REQUIRE(args);
        CHECK(args->keep);
        CHECK(args->jobs == 4);
    }
    SUBCASE("-kj4 -k") {
        const auto args = parse_from<Jobs>({"-kj4", "-k"});
        REQUIRE(args);
        CHECK(args->keep);
        CHECK(args->jobs == 4);
    }
    SUBCASE("-j42") {
        const auto args = parse_from<Jobs>({"-j42"});
        REQUIRE(args);
        CHECK_FALSE(args->keep);
        CHECK(args->jobs == 42);
    }
}

TEST_CASE("bundle: single-dash full names are not bundles") {
    const auto args = parse_from<Jobs>({"-keep", "-jobs", "4"});
    REQUIRE(args);
    CHECK(args->keep);
    CHECK(args->jobs == 4);
}

TEST_CASE("bundle: a value-taking option without a value counts once") {
    const auto args = parse_from<Jobs>({"-kj"});
    REQUIRE(args);
    CHECK(args->keep);
    CHECK(args->jobs == 1);
}

TEST_CASE("bundle: aliases work inside bundles") {
    SUBCASE("-qj4") {
        const auto args = parse_from<Jobs>({"-qj4"});
        REQUIRE(args);
        CHECK(args->keep);
        CHECK(args->jobs == 4);
    }
    SUBCASE("-kn4") {
        const auto args = parse_from<Jobs>({"-kn4"});
        REQUIRE(args);
        CHECK(args->keep);
        CHECK(args->jobs == 4);
    }
    SUBCASE("-QN4 (case-insensitive)") {
        const auto args = parse_from<Jobs>({"-QN4"});
        REQUIRE(args);
        CHECK(args->keep);
        CHECK(args->jobs == 4);
    }
}

TEST_CASE("bundle: trailing characters after an attached value are an error") {
    const auto args = parse_from<Jobs>({"-kj4k"});
    REQUIRE_FALSE(args);
    CHECK(args.error_message() == "Could not parse input '4k' for argument 'int'");
}

TEST_CASE("bundle: an unknown character rejects the whole bundle") {
    const auto args = parse_from<Jobs>({"-kxj4"});
    REQUIRE_FALSE(args);
    CHECK(args.error_message() == "Unknown option 'kxj4'");
}

TEST_CASE("bundle: double-dash tokens are never expanded as bundles") {
    const auto args = parse_from<Jobs>({"--kj4"});
    REQUIRE_FALSE(args);
    CHECK(args.error_message() == "Unknown option 'kj4'");
}

TEST_CASE("bundle: attached string value") {
    SUBCASE("-kvofile") {
        const auto args = parse_from<Values>({"-kvofile"});
        REQUIRE(args);
        CHECK(args->kay);
        CHECK(args->verbose);
        CHECK(args->output_file == "file");
    }
    SUBCASE("-kokvf (the value may contain flag characters)") {
        const auto args = parse_from<Values>({"-kokvf"});
        REQUIRE(args);
        CHECK(args->kay);
        CHECK_FALSE(args->verbose);
        CHECK(args->output_file == "kvf");
    }
}

TEST_CASE("bundle: positionals after a bundle") {
    SUBCASE("-kvofile input") {
        const auto args = parse_from<Values>({"-kvofile", "input"});
        REQUIRE(args);
        CHECK(args->arg1 == "input");
        CHECK(args->output_file == "file");
    }
    SUBCASE("-kvo file input") {
        const auto args = parse_from<Values>({"-kvo", "file", "input"});
        REQUIRE(args);
        CHECK(args->arg1 == "input");
        CHECK(args->output_file == "file");
    }
}

TEST_CASE("bundle: attached container values accumulate") {
    const auto args = parse_from<Values>({"-kIpath1", "-Ipath2"});
    REQUIRE(args);
    CHECK(args->kay);
    const std::vector<std::string> expected{"path1", "path2"};
    CHECK(args->include_paths == expected);
}
