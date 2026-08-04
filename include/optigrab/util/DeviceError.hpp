#pragma once

#include <string>

namespace optigrab {

// Human-readable hints for optical-device failures (permissions, empty tray, busy, etc.).
// errnoOrWin32: POSIX errno, or Win32 GetLastError() value when on Windows adapters.
[[nodiscard]] std::string describeDeviceFailure(const std::string& devicePath,
                                                int errnoOrWin32,
                                                const std::string& operation);

// Convenience when errno is already set (Linux/macOS open failures).
[[nodiscard]] std::string describeDeviceFailureFromErrno(const std::string& devicePath,
                                                         const std::string& operation);

}  // namespace optigrab
