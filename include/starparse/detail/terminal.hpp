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

    inline constexpr std::string_view whitespace = " \t\r";

    constexpr void wrap_line(std::string_view line, const std::size_t width, std::size_t hang, std::string &out) {
        const auto skip = [line](const std::size_t from, const bool space) {
            return std::min(space ? line.find_first_not_of(whitespace, from) : line.find_first_of(whitespace, from),
                            line.size());
        };

        std::size_t pos = skip(0, true);
        const auto fits = [width](const std::size_t n) { return n <= width / 2; };
        const std::size_t indent = fits(pos) ? pos : 0;
        if (!fits(hang)) hang = indent;

        out.append(line.substr(0, indent));
        std::size_t col = indent;
        std::size_t gap = 0;

        while (pos < line.size()) {
            const std::size_t end = skip(pos, false);
            const std::string_view word = line.substr(pos, end - pos);

            if (gap > 0) {
                if (col + gap + word.size() > width) {
                    out += '\n';
                    out.append(hang, ' ');
                    col = hang;
                } else {
                    out.append(line.substr(pos - gap, gap));
                    col += gap;
                }
            }

            const std::size_t room = width - col;
            if (word.size() <= room) out += word;
            else if (room > 3) out.append(word.substr(0, room - 3)).append("...");
            else out.append(room, '.');
            col += std::min(word.size(), room);

            pos = skip(end, true);
            gap = pos - end;
        }
    }

    constexpr std::string wrap(std::string_view text, const int width = terminal_width(),
                               const std::size_t hang = std::string_view::npos) {
        const std::size_t w = width > 0 ? static_cast<std::size_t>(width) : 1;
        std::string out;
        out.reserve(text.size() + text.size() / w);

        bool first = true;
        for (auto line : text | std::views::split('\n')) {
            if (!std::exchange(first, false)) out += '\n';
            wrap_line(std::string_view{line}, w, hang, out);
        }
        return out;
    }
}
