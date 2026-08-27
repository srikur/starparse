set(CMAKE_CXX_EXTENSIONS OFF)

function(starparse_test_executable name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE starparse::starparse)
    target_compile_options(${name} PRIVATE -Wall -Werror)
endfunction()

starparse_test_executable(test_parse)
starparse_test_executable(test_bare)
starparse_test_executable(test_positional_named)
starparse_test_executable(test_bundle)
starparse_test_executable(test_prefill)
starparse_test_executable(test_errors)
starparse_test_executable(test_alias)
starparse_test_executable(test_enum)

# starparse_case(<test name> <target> <expected output regex> [args...])
function(starparse_case name target expected)
    add_test(NAME ${name} COMMAND ${target} ${ARGN})
    set_tests_properties(${name} PROPERTIES PASS_REGULAR_EXPRESSION "${expected}")
endfunction()

# Annotated struct: only annotated fields participate.
starparse_case(parse.no_args     test_parse "k: false, v: false, f: false")
starparse_case(parse.flag_bundle test_parse "k: true, v: true, f: true" -kvf)
starparse_case(parse.long_flag   test_parse "k: false, v: true, f: false" --verbose)
starparse_case(parse.short_flags test_parse "k: true, v: false, f: true" -k -f)
starparse_case(parse.enum_value  test_parse "mode: 2" -m MODE_2)

# Bare struct: every field addressable by name, non-bools also positional.
starparse_case(bare.named_mixed_dashes test_bare "Bare arg1: hello, verbose: false, arg2: 42" --arg1=hello -arg2=42)
starparse_case(bare.positional         test_bare "Bare arg1: hello, verbose: false, arg2: 42" hello 42)
starparse_case(bare.flag               test_bare "Bare arg1: hello, verbose: true, arg2: 42" hello --verbose 42)

# Positional annotations are also addressable by full name.
starparse_case(positional.by_order test_positional_named "Args arg1: hello, verbose: true, arg2: 42" hello -v 42)
starparse_case(positional.by_name  test_positional_named "Args arg1: hello, verbose: false, arg2: 42" --arg1 hello --arg2 42)

# --name=value and -kvf bundles.
starparse_case(bundle.flags_and_positionals test_bundle "arg1: hello, keep: true, verbose: true, force: true, arg2: 42" -kvf hello 42)
starparse_case(bundle.equals                test_bundle "arg1: hello, keep: false, verbose: false, force: false, arg2: 42" --arg1=hello --arg2=42)

# Prefilled object: only fields mentioned on the command line are overwritten.
starparse_case(prefill.defaults_survive    test_prefill "arg1: default1, keep: true, verbose: false, arg2: 99")
starparse_case(prefill.partial_override    test_prefill "arg1: default1, keep: true, verbose: true, arg2: 5" --arg2=5 -v)
starparse_case(prefill.positional_override test_prefill "arg1: hello, keep: true, verbose: false, arg2: 99" hello)

# Alias annotations: extra names for options and positionals.
starparse_case(alias.long_alias     test_alias "input: hello, verbose: true, keep: false, arg2: 42" --talkative hello 42)
starparse_case(alias.named_by_alias test_alias "input: hello, verbose: false, keep: false, arg2: 42" --source=hello --count=42)
starparse_case(alias.single_char    test_alias "input: hello, verbose: true, keep: false, arg2: 42" -q hello 42)
starparse_case(alias.bundle_char    test_alias "input: hello, verbose: true, keep: true, arg2: 42" -qk hello 42)

# Enum values match by enumerator name or by Alias annotations on enumerators.
starparse_case(enum.by_name     test_enum "mode: 1, input: hello" --mode=safe hello)
starparse_case(enum.by_alias    test_enum "mode: 0, input: hello" --mode quick hello)
starparse_case(enum.alias_dry   test_enum "mode: 2, input: hello" -m dry hello)
starparse_case(enum.alias_short test_enum "mode: 2, input: hello" -m n hello)
starparse_case(enum.bad_value   test_enum "error: bad value for type: Mode" --mode=slow hello)

# Errors surface as exceptions for now.
starparse_case(errors.unrecognized  test_errors "error: unrecognized argument: nope" --nope)
starparse_case(errors.flag_value    test_errors "error: option does not take a value: verbose" --verbose=true)
starparse_case(errors.missing_value test_errors "error: missing value for option: arg1" --arg1)
starparse_case(errors.bad_value     test_errors "error: bad value for type: int" abc)
