#pragma once

#include <doctest/doctest.h>
#include <starparse/starparse.hpp>

#include <format>
#include <string>

// when REQUIRE(args) / CHECK(args) fails, show the parse errors instead of "{?}"
template<typename T>
struct doctest::StringMaker<StarParse::ParsedArgs<T> > {
    static String convert(const StarParse::ParsedArgs<T> &args) {
        if (args) { return "parsed"; }
        const std::string message = args.error_message();
        return message.c_str();
    }
};

template<typename T>
    requires std::formattable<T, char>
struct doctest::StringMaker<T> {
    static String convert(const T &value) {
        const std::string text = std::format("{}", value);
        return text.c_str();
    }
};
