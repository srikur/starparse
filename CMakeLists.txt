cmake_minimum_required(VERSION 3.25)

project(starparse
    VERSION 0.0.2
    DESCRIPTION "C++26 reflection-based command line argument parser"
    HOMEPAGE_URL "https://github.com/srikur/starparse"
    LANGUAGES CXX)

option(STARPARSE_BUILD_TESTS "Build the starparse tests" ${PROJECT_IS_TOP_LEVEL})
option(STARPARSE_BUILD_EXAMPLES "Build the starparse examples" ${PROJECT_IS_TOP_LEVEL})
option(STARPARSE_INSTALL "Generate install rules for starparse" ${PROJECT_IS_TOP_LEVEL})

include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)

add_library(starparse INTERFACE)
add_library(starparse::starparse ALIAS starparse)

target_compile_features(starparse INTERFACE cxx_std_26)

if(NOT "cxx_std_26" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    message(FATAL_ERROR
        "starparse requires a C++26 compiler. "
        "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} (${CMAKE_CXX_COMPILER}) does not report C++26 support. "
        "Point CMake at another compiler with -DCMAKE_CXX_COMPILER=<path> or the CXX environment variable.")
endif()

function(starparse_detect_reflection flag_out found_out)
    set(CMAKE_CXX_STANDARD 26)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    set(flag "")
    check_cxx_compiler_flag(-freflection STARPARSE_HAS_FREFLECTION)
    if(STARPARSE_HAS_FREFLECTION)
        set(flag -freflection)
    else()
        check_cxx_compiler_flag(-freflection-latest STARPARSE_HAS_FREFLECTION_LATEST)
        if(STARPARSE_HAS_FREFLECTION_LATEST)
            set(flag -freflection-latest)
        endif()
    endif()

    set(CMAKE_REQUIRED_FLAGS "${flag}")
    check_cxx_source_compiles([[
#include <meta>
#include <string_view>
struct Probe {};
static_assert(std::meta::identifier_of(^^Probe) == std::string_view{"Probe"});
int main() {}
]] STARPARSE_HAS_REFLECTION)

    set(${flag_out} "${flag}" PARENT_SCOPE)
    set(${found_out} "${STARPARSE_HAS_REFLECTION}" PARENT_SCOPE)
endfunction()

# needed for std::println support in MinGW (libstdc++ on MinGW: libstdc++exp)
function(starparse_detect_print_libs libs_out)
    set(CMAKE_CXX_STANDARD 26)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    set(probe [[
#include <print>
int main() { std::println("{}", 42); }
]])

    set(libs "")
    check_cxx_source_compiles("${probe}" STARPARSE_PRINT_LINKS)
    if(NOT STARPARSE_PRINT_LINKS)
        set(CMAKE_REQUIRED_LIBRARIES stdc++exp)
        check_cxx_source_compiles("${probe}" STARPARSE_PRINT_LINKS_WITH_STDCXXEXP)
        if(STARPARSE_PRINT_LINKS_WITH_STDCXXEXP)
            set(libs stdc++exp)
        endif()
    endif()

    set(${libs_out} "${libs}" PARENT_SCOPE)
endfunction()

starparse_detect_reflection(STARPARSE_REFLECTION_FLAG STARPARSE_HAS_REFLECTION)
starparse_detect_print_libs(STARPARSE_PRINT_LIBS)

if(STARPARSE_REFLECTION_FLAG)
    target_compile_options(starparse INTERFACE ${STARPARSE_REFLECTION_FLAG})
endif()

if(STARPARSE_PRINT_LIBS)
    target_link_libraries(starparse INTERFACE ${STARPARSE_PRINT_LIBS})
    message(STATUS "starparse: linking ${STARPARSE_PRINT_LIBS} for std::println support")
endif()

if(STARPARSE_HAS_REFLECTION)
    set(_starparse_flag_note "")
    if(STARPARSE_REFLECTION_FLAG)
        set(_starparse_flag_note " with ${STARPARSE_REFLECTION_FLAG}")
    endif()
    unset(_starparse_flag_note)
else()
    message(WARNING
        "starparse is built using reflection (C++26 <meta>). "
        "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} (${CMAKE_CXX_COMPILER}) does not provide it, "
        "so targets that include starparse will fail to compile.")
endif()

target_sources(starparse INTERFACE
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
        include/starparse/starparse.hpp
        include/starparse/detail/annotations.hpp
        include/starparse/detail/assertions.hpp
        include/starparse/detail/errors.hpp
        include/starparse/detail/file.hpp
        include/starparse/detail/parser.hpp
        include/starparse/detail/settings.hpp
        include/starparse/detail/terminal.hpp
        include/starparse/detail/utilities.hpp)

if(STARPARSE_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    enable_testing()
    add_subdirectory(tests)
endif()

if(STARPARSE_BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()

if(STARPARSE_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    install(TARGETS starparse EXPORT starparseTargets FILE_SET HEADERS)

    install(EXPORT starparseTargets
        FILE starparseTargets.cmake
        NAMESPACE starparse::
        DESTINATION ${CMAKE_INSTALL_DATADIR}/cmake/starparse)

    configure_package_config_file(cmake/starparseConfig.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/starparseConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/cmake/starparse)

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/starparseConfigVersion.cmake"
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT)

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/starparseConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/starparseConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/cmake/starparse)
endif()
