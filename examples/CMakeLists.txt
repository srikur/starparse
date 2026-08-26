set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(basic basic.cpp)
target_link_libraries(basic PRIVATE starparse::starparse)
target_compile_options(basic PRIVATE -Wall -Werror)
