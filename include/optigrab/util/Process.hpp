#pragma once

#include "optigrab/domain/Errors.hpp"

#include <array>
#include <cstdio>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <vector>

namespace optigrab {

// Run a process, capture combined-ish stderr via redirected command.
// Returns exit code. Throws nothing; caller checks code.
inline int runProcess(const std::vector<std::string>& args, std::string& stdoutOut,
                      std::string& stderrOut) {
    if (args.empty()) {
        return 127;
    }

    std::ostringstream cmd;
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (i) {
            cmd << ' ';
        }
        // Single-quote shell-escape.
        cmd << '\'';
        for (char c : args[i]) {
            if (c == '\'') {
                cmd << "'\\''";
            } else {
                cmd << c;
            }
        }
        cmd << '\'';
    }
    // Merge stderr to a temp capture via bash process substitution is hard;
    // use 2>&1 and split is lossy — capture all as stderrOut and leave stdout empty
    // when we only care about success. For simplicity: redirect stderr to stdout.
    cmd << " 2>&1";

    stdoutOut.clear();
    stderrOut.clear();
    FILE* pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) {
        stderrOut = "popen failed";
        return 127;
    }
    std::array<char, 4096> buffer{};
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        stdoutOut += buffer.data();
    }
    const int status = pclose(pipe);
    if (status == -1) {
        return 127;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return 1;
}

inline void runProcessOrThrow(const std::vector<std::string>& args, const std::string& what) {
    std::string out;
    std::string err;
    const int code = runProcess(args, out, err);
    if (code != 0) {
        std::string msg = what + " failed (exit " + std::to_string(code) + ")";
        if (!out.empty()) {
            // last line-ish
            msg += ": " + out.substr(0, std::min<std::size_t>(out.size(), 400));
        }
        throw OptigrabError(msg);
    }
}

}  // namespace optigrab
