#include "optigrab/cli/LineReader.hpp"

#include <cstdio>
#include <termios.h>
#include <unistd.h>

namespace optigrab {
namespace {

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
    // Clear current line and rewrite prompt + buffer.
    out << "\r\033[K" << prompt << line << std::flush;
}

bool readByte(int fd, char& c) {
    const ssize_t n = ::read(fd, &c, 1);
    return n == 1;
}

}  // namespace

LineReader::LineReader(History& history, std::ostream& out, std::istream& in)
    : history_(history), out_(out), in_(in) {}

std::optional<std::string> LineReader::readLine(const std::string& prompt) {
    if (&in_ == &std::cin && ::isatty(STDIN_FILENO) && ::isatty(STDOUT_FILENO)) {
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
    RawMode raw(STDIN_FILENO);
    if (!raw.active()) {
        out_ << prompt << std::flush;
        return readLinePlain();
    }

    std::string line;
    out_ << prompt << std::flush;

    while (true) {
        char c = 0;
        if (!readByte(STDIN_FILENO, c)) {
            out_ << "\n" << std::flush;
            return std::nullopt;
        }

        if (c == '\r' || c == '\n') {
            out_ << "\n" << std::flush;
            return line;
        }

        if (c == 3) {  // Ctrl-C: cancel line
            line.clear();
            out_ << "^C\n" << std::flush;
            return std::string{};
        }

        if (c == 4) {  // Ctrl-D
            if (line.empty()) {
                out_ << "\n" << std::flush;
                return std::nullopt;
            }
            continue;
        }

        if (c == 127 || c == 8) {  // Backspace / BS
            if (!line.empty()) {
                line.pop_back();
                redrawLine(out_, prompt, line);
            }
            continue;
        }

        if (c == 27) {  // ESC sequence (arrows)
            char seq[2]{};
            if (!readByte(STDIN_FILENO, seq[0])) {
                continue;
            }
            if (!readByte(STDIN_FILENO, seq[1])) {
                continue;
            }
            if (seq[0] == '[') {
                if (seq[1] == 'A') {  // Up
                    if (const auto prev = history_.older(line)) {
                        line = *prev;
                        redrawLine(out_, prompt, line);
                    }
                } else if (seq[1] == 'B') {  // Down
                    if (const auto next = history_.newer()) {
                        line = *next;
                        redrawLine(out_, prompt, line);
                    }
                }
                // Ignore left/right for now (cursor always at end).
            }
            continue;
        }

        // Printable ASCII (skip other controls).
        if (c >= 32 && c < 127) {
            line.push_back(c);
            out_ << c << std::flush;
        }
    }
}

}  // namespace optigrab
