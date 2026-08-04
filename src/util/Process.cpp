#include "optigrab/util/Process.hpp"

#include <array>
#include <cstdio>
#include <sstream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/wait.h>
#endif

namespace optigrab {
namespace {

std::string quoteArg(const std::string& arg) {
#ifdef _WIN32
    // Minimal Windows quoting for CreateProcess-style command lines via _popen.
    if (arg.find_first_of(" \t\"") == std::string::npos) {
        return arg;
    }
    std::string out = "\"";
    for (char c : arg) {
        if (c == '"') {
            out += "\\\"";
        } else {
            out += c;
        }
    }
    out += '"';
    return out;
#else
    std::string out = "'";
    for (char c : arg) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out += c;
        }
    }
    out += '\'';
    return out;
#endif
}

}  // namespace

int runProcess(const std::vector<std::string>& args, std::string& stdoutOut,
               std::string& stderrOut) {
    if (args.empty()) {
        return 127;
    }

    std::ostringstream cmd;
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (i) {
            cmd << ' ';
        }
        cmd << quoteArg(args[i]);
    }
#ifdef _WIN32
    cmd << " 2>&1";
#else
    cmd << " 2>&1";
#endif

    stdoutOut.clear();
    stderrOut.clear();

#ifdef _WIN32
    FILE* pipe = _popen(cmd.str().c_str(), "r");
#else
    FILE* pipe = popen(cmd.str().c_str(), "r");
#endif
    if (!pipe) {
        stderrOut = "popen failed";
        return 127;
    }

    std::array<char, 4096> buffer{};
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        stdoutOut += buffer.data();
    }

#ifdef _WIN32
    const int status = _pclose(pipe);
    return status;
#else
    const int status = pclose(pipe);
    if (status == -1) {
        return 127;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return 1;
#endif
}

void runProcessOrThrow(const std::vector<std::string>& args, const std::string& what) {
    std::string out;
    std::string err;
    const int code = runProcess(args, out, err);
    if (code != 0) {
        std::string msg = what + " failed (exit " + std::to_string(code) + ")";
        if (!out.empty()) {
            msg += ": " + out.substr(0, std::min<std::size_t>(out.size(), 400));
        }
        throw OptigrabError(msg);
    }
}

}  // namespace optigrab
