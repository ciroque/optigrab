#include "optigrab/cli/LineReader.hpp"

#include <cstdio>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <io.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace optigrab {
namespace {

#ifdef _WIN32

bool stdinIsTty() { return _isatty(_fileno(stdin)) != 0; }
bool stdoutIsTty() { return _isatty(_fileno(stdout)) != 0; }

// Windows console: enable VT if possible; use ReadConsole for basic editing.
// Arrow-key history uses VK_UP/VK_DOWN via ReadConsoleInput.

void redrawLine(std::ostream& out, const std::string& prompt, const std::string& line) {
    out << "\r\033[K" << prompt << line << std::flush;
}

std::optional<std::string> readLineInteractiveWin(History& history, std::ostream& out,
                                                  const std::string& prompt) {
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hIn == INVALID_HANDLE_VALUE || hOut == INVALID_HANDLE_VALUE) {
        return std::nullopt;
    }

    DWORD oldMode = 0;
    GetConsoleMode(hIn, &oldMode);
    // Process input so we get key events; disable line input/echo (we paint ourselves).
    SetConsoleMode(hIn, ENABLE_PROCESSED_INPUT | ENABLE_WINDOW_INPUT);

    DWORD outMode = 0;
    GetConsoleMode(hOut, &outMode);
    SetConsoleMode(hOut, outMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    std::string line;
    out << prompt << std::flush;

    while (true) {
        INPUT_RECORD rec{};
        DWORD read = 0;
        if (!ReadConsoleInputA(hIn, &rec, 1, &read) || read == 0) {
            SetConsoleMode(hIn, oldMode);
            out << "\n" << std::flush;
            return std::nullopt;
        }
        if (rec.EventType != KEY_EVENT || !rec.Event.KeyEvent.bKeyDown) {
            continue;
        }

        const auto& key = rec.Event.KeyEvent;
        const WORD vk = key.wVirtualKeyCode;
        const char ch = key.uChar.AsciiChar;

        if (vk == VK_RETURN) {
            out << "\n" << std::flush;
            SetConsoleMode(hIn, oldMode);
            return line;
        }
        if (vk == VK_BACK) {
            if (!line.empty()) {
                line.pop_back();
                redrawLine(out, prompt, line);
            }
            continue;
        }
        if (vk == VK_UP) {
            if (const auto prev = history.older(line)) {
                line = *prev;
                redrawLine(out, prompt, line);
            }
            continue;
        }
        if (vk == VK_DOWN) {
            if (const auto next = history.newer()) {
                line = *next;
                redrawLine(out, prompt, line);
            }
            continue;
        }
        if (vk == 'C' && (key.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))) {
            line.clear();
            out << "^C\n" << std::flush;
            SetConsoleMode(hIn, oldMode);
            return std::string{};
        }
        if (vk == 'Z' && (key.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))) {
            // Treat as EOF-ish cancel of empty line
            if (line.empty()) {
                out << "\n" << std::flush;
                SetConsoleMode(hIn, oldMode);
                return std::nullopt;
            }
            continue;
        }

        if (ch >= 32 && ch < 127) {
            line.push_back(ch);
            out << ch << std::flush;
        }
    }
}

#else

bool stdinIsTty() { return ::isatty(STDIN_FILENO) != 0; }
bool stdoutIsTty() { return ::isatty(STDOUT_FILENO) != 0; }

class RawMode {
public:
    explicit RawMode(int fd) : fd_(fd), active_(false) {
        if (!::isatty(fd_)) {
            return;
        }
        if (::tcgetattr(fd_, &original_) != 0) {
            return;
        }
        termios raw = original_;
        raw.c_lflag &= static_cast<tcflag_t>(~(ECHO | ICANON | IEXTEN));
        raw.c_iflag &= static_cast<tcflag_t>(~(IXON | ICRNL));
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        if (::tcsetattr(fd_, TCSAFLUSH, &raw) == 0) {
            active_ = true;
        }
    }

    ~RawMode() {
        if (active_) {
            ::tcsetattr(fd_, TCSAFLUSH, &original_);
        }
    }

    RawMode(const RawMode&) = delete;
    RawMode& operator=(const RawMode&) = delete;

    [[nodiscard]] bool active() const { return active_; }

private:
    int fd_;
    termios original_{};
    bool active_;
};

void redrawLine(std::ostream& out, const std::string& prompt, const std::string& line) {
    out << "\r\033[K" << prompt << line << std::flush;
}

bool readByte(int fd, char& c) {
    const ssize_t n = ::read(fd, &c, 1);
    return n == 1;
}

std::optional<std::string> readLineInteractivePosix(History& history, std::ostream& out,
                                                    std::istream& in, const std::string& prompt) {
    RawMode raw(STDIN_FILENO);
    if (!raw.active()) {
        out << prompt << std::flush;
        std::string line;
        if (!std::getline(in, line)) {
            return std::nullopt;
        }
        return line;
    }

    std::string line;
    out << prompt << std::flush;

    while (true) {
        char c = 0;
        if (!readByte(STDIN_FILENO, c)) {
            out << "\n" << std::flush;
            return std::nullopt;
        }

        if (c == '\r' || c == '\n') {
            out << "\n" << std::flush;
            return line;
        }
        if (c == 3) {
            line.clear();
            out << "^C\n" << std::flush;
            return std::string{};
        }
        if (c == 4) {
            if (line.empty()) {
                out << "\n" << std::flush;
                return std::nullopt;
            }
            continue;
        }
        if (c == 127 || c == 8) {
            if (!line.empty()) {
                line.pop_back();
                redrawLine(out, prompt, line);
            }
            continue;
        }
        if (c == 27) {
            char seq[2]{};
            if (!readByte(STDIN_FILENO, seq[0]) || !readByte(STDIN_FILENO, seq[1])) {
                continue;
            }
            if (seq[0] == '[') {
                if (seq[1] == 'A') {
                    if (const auto prev = history.older(line)) {
                        line = *prev;
                        redrawLine(out, prompt, line);
                    }
                } else if (seq[1] == 'B') {
                    if (const auto next = history.newer()) {
                        line = *next;
                        redrawLine(out, prompt, line);
                    }
                }
            }
            continue;
        }
        if (c >= 32 && c < 127) {
            line.push_back(c);
            out << c << std::flush;
        }
    }
}

#endif

}  // namespace

LineReader::LineReader(History& history, std::ostream& out, std::istream& in)
    : history_(history), out_(out), in_(in) {}

std::optional<std::string> LineReader::readLine(const std::string& prompt) {
    if (&in_ == &std::cin && stdinIsTty() && stdoutIsTty()) {
        return readLineInteractive(prompt);
    }
    out_ << prompt << std::flush;
    return readLinePlain();
}

std::optional<std::string> LineReader::readLinePlain() {
    std::string line;
    if (!std::getline(in_, line)) {
        return std::nullopt;
    }
    return line;
}

std::optional<std::string> LineReader::readLineInteractive(const std::string& prompt) {
#ifdef _WIN32
    return readLineInteractiveWin(history_, out_, prompt);
#else
    return readLineInteractivePosix(history_, out_, in_, prompt);
#endif
}

}  // namespace optigrab
