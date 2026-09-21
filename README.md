# starparse

[![CI](https://github.com/srikur/starparse/actions/workflows/ci.yml/badge.svg)](https://github.com/srikur/starparse/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

A header-only command line argument parser built on C++26 reflection. Describe your arguments as a plain struct,
annotate the fields (or don't), and parse.

```cpp
#include <print>
#include <starparse/starparse.hpp>

struct [[=StarParse::Program{"archiver", "Compress files", "1.0.0"}]] Options {
    [[=StarParse::Positional{0}, =StarParse::Required]] std::string input;
    [[=StarParse::Positional{1}]] std::string output;
    [[=StarParse::Opt{'v', "Enable verbose logging"}]] bool verbose;
    [[=StarParse::Opt{'j', "Number of worker threads"}, =StarParse::Range{1, 64}]] int jobs;
};

auto main(int argc, char** argv) -> int {
    Options defaults{.output = "archive.out", .jobs = 1};
    const auto opts{StarParse::parse_or_exit(argc, argv, defaults)};
    if (opts.help_requested()) { std::println("{}", opts.help()); return 0; }
    if (opts.version_requested()) { std::println("{}", opts.version()); return 0; }
    // opts->input, opts->output, opts->verbose, opts->jobs
}
```

## Requirements

- **GCC 16 or newer** (`-std=c++26 -freflection`).
- CMake >= 3.25 to build the tests/examples or install the package. No dependencies other than the standard library.

## Features

### Naming and Matching

Every annotated field is addressable by its full name in several spellings: `--jobs 4`, `--jobs=4`, `-jobs 4`,
`-jobs=4`. `Opt` adds a one-character short name (`-j 4`, `-j=4`).

- **Kebab-case**: a field named `dry_run` also resolves via `--dry-run` (on by default).
- **Case-insensitive**: `--Jobs` and `--JOBS` match `jobs` (on by default).
- **Aliases**: `[[=StarParse::Alias{"threads", "t"}]]` adds any number of extra long or short spellings. Aliases are
  listed in the generated help text.

All of these are individually switchable via [`Settings`](#settings).

### Bare Structs

A struct with **no** `Opt`/`Positional` annotations is "bare": every field is addressable by name, and non-bool fields
also fill positionally in declaration order.

```cpp
struct Options { std::string input; std::string output; bool verbose; };
// prog in.txt out.txt --verbose
```

### Positional Arguments

`Positional{n}` fields fill by position (indices must start at 0 and be contiguous) and remain addressable by name.
Everything after a standalone `--` is treated as positional, and a lone `-` is treated as a positional value rather than
an option. Container fields cannot be positional.

### Flags

- **Bundling**: `-kvf` sets three flags at once.
- **Attached values**: a bundle may end in one value-taking option — `-kj4` sets `keep` and `jobs=4`, and negative
  numbers work (`-kj-4`).
- **Explicit values**: `--verbose=yes` / `--verbose=off` — accepts `yes/no`, `true/false`, `t/f`, `on/off`, `1/0`,
  case-insensitively.
- **Negation**: every bool flag automatically gets a `--no-<name>` spelling that sets it to `false` (on by default via
  `autogenerate_negations`).

### Count Options

Naming an integer option *without* a value increments it, so `-j -j -j` (or `-kj` inside a bundle) yields 3. Overflow is
detected and reported. Giving a value (`-j 8`) assigns as usual. Toggleable via `allow_repeated_counts`.

### Supported Field Types

| Type                                                              | Behavior                                                                                                 |
|-------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------|
| `std::string` (or anything constructible from `std::string_view`) | value taken verbatim                                                                                     |
| integers & floating point                                         | parsed with `std::from_chars`; the whole token must parse                                                |
| `bool`                                                            | flag semantics (see above)                                                                               |
| `char` and other character types                                  | first character of the value                                                                             |
| enums                                                             | matched by enumerator name, including kebab-case, case-insensitive, and per-enumerator `Alias` spellings |
| `std::optional<T>`                                                | any of the above                                                                                         |
| `std::vector<T>`                                                  | growable list                                                                                            |
| `std::array<T, N>`                                                | exactly N values required. anything but N results in an error                                            |
| anything else                                                     | via a custom [`Parser`](#custom-parsers) annotation                                                      |

```cpp
enum class Speed {
    fast [[=StarParse::Alias{"quick"}]],
    slow,
};
// --speed fast, --speed quick, --speed FAST all work
```

### Containers and value separators

Container fields accept repeated options (`-v hello -v world`), separator-delimited values (`--nums 1,2,3`), or a mix of
the two. The separator defaults to `,` and can be changed globally (`Settings::value_separator`) or per field:

```cpp
[[=StarParse::Opt{'a'}, =StarParse::Separator{";"}]] std::array<std::string, 4> parts;
```

A prefilled default `vector` is cleared the first time the option appears on the command line, so command-line values
replace defaults rather than appending to them.

### Defaults and prefilled objects

Pass an existing object as the third argument to `parse` or its siblings and only the fields mentioned on the command
line are overwritten—everything else keeps the value you set.

### Required fields

`[[=StarParse::Required]]` makes omission a parse error (`Missing value for required option '...'`) and marks the field
`(required)` in the help text. Combining `Required` with `std::optional` is rejected at compile time.

### Constraints and validation

At most one of these per field (enforced at compile time), applied per element for containers:

```cpp
[[=StarParse::Opt{'j'}, =StarParse::Min{1}]]            int jobs;
[[=StarParse::Opt{'p'}, =StarParse::Max{65535}]]        int port;
[[=StarParse::Opt{'l'}, =StarParse::Range{0, 255}]]     uint8_t level;
[[=StarParse::Choices{"json", "yaml", "toml"}]]         std::string format;
[[=StarParse::Validator{[](const std::string &s) -> bool { return !s.empty(); }}]] std::string name;
```

`Choices` works for string-like and numeric/enum types. `Validator` accepts three function shapes:

- `bool(const T&)` — `false` means invalid;
- `const char*(const T&)` — return a message, or `nullptr` for valid;
- `std::expected<void, std::string>(const T&)` — full control over the error text.

Failures are reported with the offending value and the constraint (`minimum is 1`,
`allowed choices are ["json", "yaml", "toml"]`, or your validator's message).

### Custom parsers

The `Parser` annotation allows you to convert the raw string yourself, which lets you use field types starparse doesn't
natively
understand:

```cpp
std::expected<std::chrono::seconds, std::string> parse_seconds(const std::string_view &s);

struct Args {
    [[=StarParse::Parser{parse_seconds}]] std::chrono::seconds timeout;
};
// --timeout 30s
```

### Subcommands

Annotate a `std::optional<ChildStruct>` field with `Subcommand`. The field name (plus kebab/case/alias variants) is the
command name; the child struct is a full argument struct of its own; it can have its own positionals, options, `Program`
description, and even nested subcommands.

```cpp
struct Add      { [[=StarParse::Positional{0}]] int value; };
struct Subtract { [[=StarParse::Positional{0}]] int value; };

struct Calc {
    [[=StarParse::Required, =StarParse::Positional{0}]] int initial;
    [[=StarParse::Subcommand, =StarParse::Alias{"plus"}]]  std::optional<Add> add;
    [[=StarParse::Subcommand, =StarParse::Alias{"minus"}]] std::optional<Subtract> subtract;
};

// calc 5 add 3   ->  opts->initial == 5, opts->add->value == 3
// calc 5 plus 3  ->  same, via the alias
```

Test which command ran by checking the optionals. Help output lists subcommands in a `Commands:` section, and
`calc add --help` prints help for the subcommand itself.

### Help and version

`--help` and `--version` are reserved. When either appears, parsing stops and the corresponding flag is set on the
result.

```cpp
const auto opts{StarParse::parse_or_exit(argc, argv)};
if (opts.help_requested())    { std::println("{}", opts.help());    return 0; }
if (opts.version_requested()) { std::println("{}", opts.version()); return 0; }
```

The generated help includes a usage line, `Arguments:`/`Options:`/`Commands:` sections, aliases, `(required)` markers,
and per-field descriptions from `Opt`/`Positional`/`Program`. The `Program` annotation on the struct supplies the
program name, description, and version string.

### Errors and the parse family

Parsing collects all errors with its kind, offending value, and argv position.
Choose how to consume them:

| Function         | On error                                                        |
|------------------|-----------------------------------------------------------------|
| `parse`          | returns normally; inspect the result                            |
| `parse_or_exit`  | prints each error to stderr and calls `std::exit(EXIT_FAILURE)` |
| `parse_or_throw` | throws `std::invalid_argument` with the first error             |
| `parse_from`     | like `parse`, but takes a brace list of arguments               |

```cpp
const auto r{StarParse::parse_from<Options>({"--jobs", "4", "in.txt"})};
if (!r) std::println(stderr, "{}", r.error_message());
```

Each function has overloads taking an initial object, `Settings`, both, or neither. The result type `ParsedArgs<T>`:

- `operator bool` — true when there were no errors;
- `operator*` / `operator->` — access the parsed struct;
- `value() &&` — move the struct out;
- `errors()` — a span of structured `ParseError` values (each has an `ErrorKind`, the input, and `to_string()`;
  `ParseError` is also `std::format`-able);
- `error_message()` — all errors joined into one string;
- `help_requested()` / `version_requested()`, `help()` / `version()`.

### Settings

Pass a `Settings` value to any parse function — via designated initializers or the chainable setters:

```cpp
StarParse::parse<Options>(argc, argv, StarParse::Settings{}
    .allowCaseInsensitivity(false)
    .setValueSeparator(";"));
```

| Setting                    | Default | Effect                                        |
|----------------------------|---------|-----------------------------------------------|
| `allow_kebab_casing`       | `true`  | `snake_case` fields also match `--kebab-case` |
| `allow_aliases`            | `true`  | honor `Alias` annotations                     |
| `allow_case_insensitivity` | `true`  | case-insensitive name/alias/enum matching     |
| `autogenerate_negations`   | `true`  | bool flags get `--no-<name>`                  |
| `allow_repeated_counts`    | `true`  | valueless integer options increment           |
| `value_separator`          | `","`   | delimiter for container values                |

### Compile-time diagnostics

There are static assert guards against misusing an annotation, with messages. Things checked, among others: duplicate
short names; non-contiguous or duplicate positional indices; `Positional` on a container; `Required` on an `optional`;
more than one of `Min`/`Max`/`Range`/`Choices`/`Validator` on a field; `Separator` on a scalar; inverted `Range` bounds;
name/alias collisions (including with the reserved `help`/`version` and each other's kebab/case spellings); and
subcommand or enum-alias collisions.

## Annotation reference

| Annotation                                  | Applies to                    | Effect                                                               |
|---------------------------------------------|-------------------------------|----------------------------------------------------------------------|
| `Opt{'x', "help"}`                          | field                         | short name and/or help text (`Opt{'x'}` and `Opt{"help"}` also work) |
| `Positional{n}` / `Positional{n, "help"}`   | field                         | fill from position `n`; still addressable by name                    |
| `Required`                                  | field                         | omission is a parse error                                            |
| `Alias{"name", ...}`                        | field or enumerator           | extra long/short spellings                                           |
| `Subcommand`                                | `std::optional<Struct>` field | subcommand                                                           |
| `Program{"name", "description", "version"}` | struct                        | powers `--help` / `--version`                                        |
| `Separator{";"}`                            | container field               | per-field value delimiter                                            |
| `Min{n}` / `Max{n}` / `Range{lo, hi}`       | numeric field                 | bounds check                                                         |
| `Choices{...}`                              | field                         | allowed-value set                                                    |
| `Validator{fn}`                             | field                         | custom check                                                         |
| `Parser{fn}`                                | field                         | custom string-to-value conversion                                    |

## Building and testing

The `release` and `debug` presets expect `g++-16` on your `PATH` (e.g. Homebrew's `gcc` package on macOS). Configure,
build, and test in one command:

```sh
cmake --workflow --preset release
```

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
cmake --preset release
cmake --install build/release        # or --prefix /some/prefix
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

Copy the `include/starparse/` directory (or download a `starparse-<version>` archive from
the [releases page](https://github.com/srikur/starparse/releases)) and compile with:

```sh
g++ -std=c++26 -freflection -I<path-to>/include your_app.cpp
```

In every case, linking against `starparse::starparse` propagates the include path, `-std=c++26`, and `-freflection`
automatically.

## License

[MIT](LICENSE)
