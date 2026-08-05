#include "optigrab/util/DeviceError.hpp"

#include <cerrno>
#include <cstring>
#include <sstream>

#ifdef _WIN32
#include "optigrab/adapters/windows/WindowsHeaders.hpp"
#endif

namespace optigrab {
namespace {

std::string joinHint(const std::string& base, const std::string& hint) {
    if (hint.empty()) {
        return base;
    }
    return base + "\n  hint: " + hint;
}

#ifdef _WIN32
std::string winHint(DWORD err) {
    switch (err) {
    case ERROR_ACCESS_DENIED:
        return "Access denied. Close other apps using the drive, or run from an elevated "
               "prompt if policy requires it.";
    case ERROR_FILE_NOT_FOUND:
    case ERROR_NOT_READY:
#ifdef ERROR_PATH_NOT_READY
    case ERROR_PATH_NOT_READY:
#endif
        return "Drive not ready. Insert an audio CD and wait for it to spin up; close Explorer "
               "preview windows that may lock the tray.";
    case ERROR_BUSY:
#ifdef ERROR_DRIVE_LOCKED
    case ERROR_DRIVE_LOCKED:
#endif
        return "Drive is busy or locked by another application (media player, burner, indexer).";
    case ERROR_INVALID_PARAMETER:
        return "Invalid device path. Use a CD-ROM drive letter like D: (see: list drive).";
    default:
        return {};
    }
}
#else
std::string posixHint(int err) {
    switch (err) {
    case EACCES:
    case EPERM:
        return "Permission denied opening the device. On Linux, add your user to the 'optical' "
               "(or 'cdrom') group, then log out and back in:  sudo usermod -aG optical $USER";
    case ENODEV:
    case ENXIO:
        return "No such device. Is the drive connected? Try: list drive";
    case ENOENT:
        return "Device path does not exist. Try: list drive";
    case EBUSY:
        return "Device is busy. Close other apps using the optical drive (players, burners).";
#ifdef ENOMEDIUM
    case ENOMEDIUM:
        return "No disc in the drive (or tray open). Insert an audio CD and retry.";
#endif
    case EIO:
        return "I/O error. Empty tray, unreadable disc, or drive problem. Check the disc is "
               "audio CD-DA and seated correctly.";
    case EAGAIN:
        return "Resource temporarily unavailable. Wait a moment and retry.";
    default:
        return {};
    }
}
#endif

}  // namespace

std::string describeDeviceFailure(const std::string& devicePath, int errnoOrWin32,
                                  const std::string& operation) {
    std::ostringstream oss;
    oss << operation << " failed on " << devicePath;

#ifdef _WIN32
    const DWORD err = static_cast<DWORD>(errnoOrWin32);
    char* msgBuf = nullptr;
    const DWORD flags =
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD size =
        FormatMessageA(flags, nullptr, err, 0, reinterpret_cast<LPSTR>(&msgBuf), 0, nullptr);
    if (size && msgBuf) {
        std::string sys(msgBuf);
        while (!sys.empty() && (sys.back() == '\r' || sys.back() == '\n' || sys.back() == ' ')) {
            sys.pop_back();
        }
        oss << " (Win32 " << err << ": " << sys << ")";
        LocalFree(msgBuf);
    } else {
        oss << " (Win32 " << err << ")";
    }
    return joinHint(oss.str(), winHint(err));
#else
    oss << " (" << std::strerror(errnoOrWin32) << " [" << errnoOrWin32 << "])";
    return joinHint(oss.str(), posixHint(errnoOrWin32));
#endif
}

std::string describeDeviceFailureFromErrno(const std::string& devicePath,
                                           const std::string& operation) {
#ifdef _WIN32
    return describeDeviceFailure(devicePath, static_cast<int>(GetLastError()), operation);
#else
    return describeDeviceFailure(devicePath, errno, operation);
#endif
}

}  // namespace optigrab
