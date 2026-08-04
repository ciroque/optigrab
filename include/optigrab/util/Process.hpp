#pragma once

#include "optigrab/domain/Errors.hpp"

#include <string>
#include <vector>

namespace optigrab {

// Run a process and capture merged stdout/stderr into stdoutOut.
// Returns exit code. Throws nothing; caller checks code.
int runProcess(const std::vector<std::string>& args, std::string& stdoutOut, std::string& stderrOut);

void runProcessOrThrow(const std::vector<std::string>& args, const std::string& what);

}  // namespace optigrab
