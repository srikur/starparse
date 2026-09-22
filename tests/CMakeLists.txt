set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG v2.5.3
        GIT_SHALLOW TRUE
        SYSTEM)
set(DOCTEST_WITH_TESTS OFF)
set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF)
set(DOCTEST_NO_INSTALL ON)
FetchContent_MakeAvailable(doctest)
include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")

add_executable(starparse_tests
        main.cpp
        test_alias.cpp
        test_bare.cpp
        test_bundle.cpp
        test_container.cpp
        test_container_validation.cpp
        test_custom_parsing.cpp
        test_entry_points.cpp
        test_enum.cpp
        test_errors.cpp
        test_help_and_version.cpp
        test_kebab.cpp
        test_name.cpp
        test_name_assertions.cpp
        test_parse.cpp
        test_positional_named.cpp
        test_prefill.cpp
        test_separator.cpp
        test_settings.cpp
        test_subcommand_assertions.cpp
        test_subcommands.cpp
        test_validations.cpp)
target_link_libraries(starparse_tests PRIVATE starparse::starparse doctest::doctest)
target_compile_options(starparse_tests PRIVATE -Wall -Werror)
doctest_discover_tests(starparse_tests)
