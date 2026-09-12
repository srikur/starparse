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
starparse_test_executable(test_try_parse)
starparse_test_executable(test_kebab)
starparse_test_executable(test_separator)
starparse_test_executable(test_container)

# starparse_case(<test name> <target> <expected output regex> [args...])
function(starparse_case name target expected)
    add_test(NAME ${name} COMMAND ${target} ${ARGN})
    set_tests_properties(${name} PROPERTIES PASS_REGULAR_EXPRESSION "${expected}")
endfunction()

# Annotated struct: only annotated fields participate.
starparse_case(parse.no_args test_parse "k: false, v: false, f: false")
starparse_case(parse.flag_bundle test_parse "k: true, v: true, f: true" -kvf)
starparse_case(parse.long_flag test_parse "k: false, v: true, f: false" --verbose)
starparse_case(parse.short_flags test_parse "k: true, v: false, f: true" -k -f)
starparse_case(parse.enum_value test_parse "mode: 2" -m MODE_2)
starparse_case(parse.attached_value test_parse "output_file: filename" -ofilename)
starparse_case(parse.vector test_parse "include_paths: \\[\"path1\", \"path2\"\\]" -I path1 -I path2)

# Bare struct: every field addressable by name, non-bools also positional.
starparse_case(bare.named_mixed_dashes test_bare "Bare arg1: hello, verbose: false, arg2: 42" --arg1=hello -arg2=42)
starparse_case(bare.positional test_bare "Bare arg1: hello, verbose: false, arg2: 42" hello 42)
starparse_case(bare.flag test_bare "Bare arg1: hello, verbose: true, arg2: 42" hello --verbose 42)

# Positional annotations are also addressable by full name.
starparse_case(positional.by_order test_positional_named "Args arg1: hello, verbose: true, arg2: 42" hello -v 42)
starparse_case(positional.by_name test_positional_named "Args arg1: hello, verbose: false, arg2: 42" --arg1 hello --arg2 42)

# --name=value and -kvf bundles.
starparse_case(bundle.flags_and_positionals test_bundle "arg1: hello, keep: true, verbose: true, force: true, arg2: 42" -kvf hello 42)
starparse_case(bundle.equals test_bundle "arg1: hello, keep: false, verbose: false, force: false, arg2: 42" --arg1=hello --arg2=42)

# Prefilled object: only fields mentioned on the command line are overwritten.
starparse_case(prefill.defaults_survive test_prefill "arg1: default1, keep: true, verbose: false, arg2: 99")
starparse_case(prefill.partial_override test_prefill "arg1: default1, keep: true, verbose: true, arg2: 5" --arg2=5 -v)
starparse_case(prefill.positional_override test_prefill "arg1: hello, keep: true, verbose: false, arg2: 99" hello)

# Alias annotations: extra names for options and positionals.
starparse_case(alias.long_alias test_alias "input: hello, verbose: true, keep: false, arg2: 42" --talkative hello 42)
starparse_case(alias.named_by_alias test_alias "input: hello, verbose: false, keep: false, arg2: 42" --source=hello --count=42)
starparse_case(alias.single_char test_alias "input: hello, verbose: true, keep: false, arg2: 42" -q hello 42)
starparse_case(alias.bundle_char test_alias "input: hello, verbose: true, keep: true, arg2: 42" -qk hello 42)

# Enum values match by enumerator name or by Alias annotations on enumerators.
starparse_case(enum.by_name test_enum "mode: 1, input: hello" --mode=safe hello)
starparse_case(enum.by_alias test_enum "mode: 0, input: hello" --mode quick hello)
starparse_case(enum.alias_dry test_enum "mode: 2, input: hello" -m dry hello)
starparse_case(enum.alias_short test_enum "mode: 2, input: hello" -m n hello)
starparse_case(enum.bad_value test_enum "error: Invalid value supplied for argument Mode \\(value: slow\\) at position 1." --mode=slow hello)

# force_parse() surfaces the first parse error as an exception.
starparse_case(errors.unrecognized test_errors "error: Unknown option supplied for argument  \\(value: nope\\) at position 1." --nope)
starparse_case(errors.missing_value test_errors "error: Missing value supplied for argument arg1 \\(value: none\\) at position 1." --arg1)
starparse_case(errors.bad_value test_errors "error: Invalid value supplied for argument int \\(value: abc\\) at position 1." abc)

# Specifically testing try_parse()
starparse_case(try_parse.single_error test_try_parse "parse error\\(s\\): \\[\"Unknown option supplied for argument  \\(value: fake_arg\\) at position 1.\"\\]" --fake_arg)
starparse_case(try_parse.successful_parse test_try_parse "verbose: true, arg1: 3" 3 --verbose)

# Kebab casing
starparse_case(kebab.basic test_kebab "dry run: 42" --dry-run=42)
starparse_case(kebab.with_value test_kebab "dry run: 42" --dry-run 42)

# -- separator
starparse_case(separator.dashed_positional test_separator "positional: --foo=bar" -- --foo=bar)

# containers
starparse_case(container.basic test_container "vector: \\[\"1\", \"2\", \"3\", \"4\"\\], array: \\[\"1\", \"2\", \"3\", \"4\"\\]" --vec=1,2,3,4 --arr=1,2,3,4)