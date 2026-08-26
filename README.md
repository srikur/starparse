# starparse

A single-header command line argument parser built using C++26's reflection. Describe your arguments as a plain struct, annotate the fields (or don't), and parse.

```cpp
#include <starparse/starparse.hpp>

struct Options {
    [[=StarParse::Positional{0}]] std::string input;
    [[=StarParse::Positional{1}]] std::string output;
    [[=StarParse::Opt{'v', "Enable verbose logging"}]] bool verbose;
    [[=StarParse::Opt{'j', "Number of worker threads"}]] int jobs;
};

auto main(int argc, char** argv) -> int {
    Options defaults{.output = "archive.out", .jobs = 1};
    const auto opts{StarParse::immediate_parse(argc, argv, defaults)};
    // opts.input, opts.output, opts.verbose, opts.jobs
}
```

- Annotated fields are addressable by full name (`--jobs 4`, `--jobs=4`, `-jobs 4`); `Opt` adds a one-char short name (`-j 4`) and bool flags support bundling (`-kvf`).
- `Positional{n}` fields also fill by position, and remain addressable by name.
- A struct with **no** annotations is "bare": every field is addressable by name, and non-bool fields fill positionally in declaration order.
- Pass an existing object as the third argument to `parse`/`immediate_parse` and only the fields mentioned on the command line are overwritten.

## Requirements

- A compiler with C++26 reflection support.
- CMake >= 3.25 to build the tests/examples or install the package. The header itself has no dependencies beyond the standard library.

## Building and testing

The `gcc` preset expects `g++-16` on your `PATH` (e.g. Homebrew's `gcc` package on macOS):

```sh
cmake --preset gcc
cmake --build --preset gcc
ctest --preset gcc
```

Without presets: `cmake -S . -B build -DCMAKE_CXX_COMPILER=g++-16` and go from there.

## Using starparse in your application

### FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(starparse
    GIT_REPOSITORY https://github.com/srikur/starparse.git
    GIT_TAG main)
FetchContent_MakeAvailable(starparse)

target_link_libraries(your_app PRIVATE starparse::starparse)
```

### Installed package

```sh
cmake --preset gcc
cmake --install build/gcc            # or --prefix /some/prefix
```

```cmake
find_package(starparse REQUIRED)
target_link_libraries(your_app PRIVATE starparse::starparse)
```

### add_subdirectory

Vendor the repo (submodule or copy) and:

```cmake
add_subdirectory(third_party/starparse)
target_link_libraries(your_app PRIVATE starparse::starparse)
```

### No CMake at all

Copy `include/starparse/starparse.hpp` and compile with:

```sh
g++ -std=c++26 -freflection -I<path-to>/include your_app.cpp
```

In every case, linking against `starparse::starparse` propagates the include path, `-std=c++26`, and `-freflection` automatically.
