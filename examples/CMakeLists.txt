set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(basic basic.cpp)
target_link_libraries(basic PRIVATE starparse::starparse)

if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(basic PRIVATE /W4 /WX)
else ()
    target_compile_options(basic PRIVATE -Wall -Werror)
endif ()
