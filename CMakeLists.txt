cmake_minimum_required(VERSION 3.25)

project(starparse
    VERSION 0.0.1
    DESCRIPTION "C++26 reflection-based command line argument parser"
    HOMEPAGE_URL "https://github.com/srikur/starparse"
    LANGUAGES CXX)

option(STARPARSE_BUILD_TESTS "Build the starparse tests" ${PROJECT_IS_TOP_LEVEL})
option(STARPARSE_BUILD_EXAMPLES "Build the starparse examples" ${PROJECT_IS_TOP_LEVEL})
option(STARPARSE_INSTALL "Generate install rules for starparse" ${PROJECT_IS_TOP_LEVEL})

add_library(starparse INTERFACE
        include/starparse/detail/annotations.h
        include/starparse/detail/settings.h
        include/starparse/detail/utilities.h
        include/starparse/detail/parser.h)
add_library(starparse::starparse ALIAS starparse)

target_compile_features(starparse INTERFACE cxx_std_26)
target_compile_options(starparse INTERFACE $<$<CXX_COMPILER_ID:GNU>:-freflection>)

target_sources(starparse INTERFACE
    FILE_SET HEADERS
    BASE_DIRS include
    FILES include/starparse/starparse.hpp)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16)
    message(WARNING
        "starparse requires P2996 reflection, which currently means GCC 16+ with -freflection. "
        "The current compiler is ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}; "
        "targets that include starparse will likely fail to compile.")
endif()

if(STARPARSE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(STARPARSE_BUILD_EXAMPLES)
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
