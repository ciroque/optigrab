#include "optigrab/adapters/windows/WindowsDriveEjector.hpp"

#include "optigrab/domain/Errors.hpp"
#include "optigrab/util/DeviceError.hpp"

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winioctl.h>

#include <cctype>
#include <string>

namespace optigrab {
namespace {

std::string toNtDevicePath(const std::string& devicePath) {
    if (devicePath.size() >= 4 && devicePath[0] == '\\' && devicePath[1] == '\\') {
        return devicePath;
    }
    if (devicePath.size() >= 2 && devicePath[1] == ':') {
        const char letter =
            static_cast<char>(std::toupper(static_cast<unsigned char>(devicePath[0])));
        return std::string("\\\\.\\") + letter + ":";
    }
    return devicePath;
}

}  // namespace

void WindowsDriveEjector::eject(const std::string& devicePath) {
    const std::string nt = toNtDevicePath(devicePath);
    HANDLE h = CreateFileA(nt.c_str(), GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        throw DriveError(describeDeviceFailure(devicePath, static_cast<int>(GetLastError()),
                                               "Open drive for eject"));
    }

    DWORD bytes = 0;
    // Allow removal, then eject.
    PREVENT_MEDIA_REMOVAL pmr{};
    pmr.PreventMediaRemoval = FALSE;
    (void)DeviceIoControl(h, IOCTL_STORAGE_MEDIA_REMOVAL, &pmr, sizeof(pmr), nullptr, 0, &bytes,
                          nullptr);

    if (!DeviceIoControl(h, IOCTL_STORAGE_EJECT_MEDIA, nullptr, 0, nullptr, 0, &bytes, nullptr)) {
        const DWORD err = GetLastError();
        CloseHandle(h);
        throw DriveError(
            describeDeviceFailure(devicePath, static_cast<int>(err), "Eject drive"));
    }
    CloseHandle(h);
}

}  // namespace optigrab

#else

namespace optigrab {

void WindowsDriveEjector::eject(const std::string& devicePath) {
    throw DriveError("WindowsDriveEjector is not available on this platform (" + devicePath + ")");
}

}  // namespace optigrab

#endif
