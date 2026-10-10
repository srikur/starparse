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
        test_duplicate_policy.cpp
        test_entry_points.cpp
        test_enum.cpp
        test_errors.cpp
        test_groups.cpp
        test_help_and_version.cpp
        test_infer.cpp
        test_kebab.cpp
        test_name.cpp
        test_parse.cpp
        test_positional_named.cpp
        test_prefill.cpp
        test_separator.cpp
        test_settings.cpp
        test_subcommand_assertions.cpp
        test_subcommands.cpp
        test_validations.cpp)
target_link_libraries(starparse_tests PRIVATE starparse::starparse doctest::doctest)
target_compile_options(starparse_tests PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wdouble-promotion
        -Wcast-align
        -Wcast-qual
        -Wold-style-cast
        -Wzero-as-null-pointer-constant
        -Wnon-virtual-dtor
        -Woverloaded-virtual
        -Wnull-dereference
        -Wimplicit-fallthrough
        -Wswitch-enum
        -Wshadow
        -Wformat=2
        -Wextra-semi
        -Wmissing-declarations
        -Wundef)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(starparse_tests PRIVATE
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            -Wuseless-cast
            -Wcast-align=strict
    )
    target_compile_options(starparse_tests PRIVATE
            -fanalyzer)

    target_compile_options(starparse_tests PRIVATE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
            -g
    )

    target_link_options(starparse_tests PRIVATE
            -fsanitize=address,undefined
    )
endif ()

doctest_discover_tests(starparse_tests)