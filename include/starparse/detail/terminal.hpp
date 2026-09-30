#pragma once
#include <string>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace StarParse::detail::Terminal {
    inline int terminal_width() {
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
            return csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
        winsize w{};
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
            return w.ws_col;
#endif
        return 80;
    }

    constexpr bool is_space(const char c) noexcept {
        return c == ' ' || c == '\t' || c == '\r';
    }

    constexpr void wrap_line(std::string_view line, const std::size_t width, std::string &out) {
        std::size_t col = 0;

        auto words = line
                     | std::views::chunk_by([](char a, char b) { return is_space(a) == is_space(b); })
                     | std::views::filter([](auto run) { return !is_space(run.front()); });

        for (std::string_view word : words
                                     | std::views::transform([](auto run) { return std::string_view{run}; })) {
            const std::size_t len = std::min(word.size(), width);

            if (col > 0) {
                if (col + 1 + len > width) {
                    out += '\n';
                    col = 0;
                } else {
                    out += ' ';
                    ++col;
                }
            }

            if (word.size() <= width) out += word;
            else if (width > 3) out.append(word.substr(0, width - 3)).append("...");
            else out.append(width, '.');

            col += len;
        }
    }

    constexpr std::string wrap(std::string_view text, const int width = terminal_width()) {
        const std::size_t w = width > 0 ? static_cast<std::size_t>(width) : 1;
        std::string out;
        out.reserve(text.size() + text.size() / w);

        bool first = true;
        for (auto line : text | std::views::split('\n')) {
            if (!std::exchange(first, false)) out += '\n';
            wrap_line(std::string_view{line}, w, out);
        }
        return out;
    }
}
