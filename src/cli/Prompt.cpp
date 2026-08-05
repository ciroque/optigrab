#include "optigrab/cli/Prompt.hpp"

#include <cctype>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace optigrab {

bool stdinIsInteractive() {
#ifdef _WIN32
    return _isatty(_fileno(stdin)) != 0;
#else
    return ::isatty(STDIN_FILENO) != 0;
#endif
}

bool promptYesNo(const std::string& question, bool defaultYes) {
    const char* hint = defaultYes ? " [Y/n] " : " [y/N] ";
    std::cout << question << hint << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) {
        return false;
    }
    // trim
    while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) {
        line.pop_back();
    }
    std::size_t start = 0;
    while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) {
        ++start;
    }
    line = line.substr(start);
    if (line.empty()) {
        return defaultYes;
    }
    const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(line[0])));
    if (c == 'y') {
        return true;
    }
    if (c == 'n') {
        return false;
    }
    // Unrecognized → default
    return defaultYes;
}

}  // namespace optigrab
