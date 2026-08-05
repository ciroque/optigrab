#include "optigrab/adapters/linux/LinuxDriveEjector.hpp"

#include "optigrab/domain/Errors.hpp"
#include "optigrab/util/DeviceError.hpp"
#include "optigrab/util/Process.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#if defined(__linux__)
#include <linux/cdrom.h>
#endif

namespace optigrab {

void LinuxDriveEjector::eject(const std::string& devicePath) {
    // Prefer the eject utility when present (handles lock/tray quirks well).
    std::string out;
    std::string err;
    if (runProcess({"eject", devicePath}, out, err) == 0) {
        return;
    }

#if defined(__linux__)
    const int fd = ::open(devicePath.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        throw DriveError(describeDeviceFailure(devicePath, errno, "Open drive for eject"));
    }
    // Unlock then eject.
    (void)::ioctl(fd, CDROM_LOCKDOOR, 0);
    if (::ioctl(fd, CDROMEJECT) != 0) {
        const int e = errno;
        ::close(fd);
        throw DriveError(describeDeviceFailure(devicePath, e, "Eject drive"));
    }
    ::close(fd);
#else
    throw DriveError("eject failed for " + devicePath +
                     (err.empty() ? out : err) +
                     " (and ioctl eject is not available on this platform build)");
#endif
}

}  // namespace optigrab
