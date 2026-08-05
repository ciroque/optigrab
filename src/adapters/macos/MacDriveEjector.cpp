#include "optigrab/adapters/macos/MacDriveEjector.hpp"

#include "optigrab/domain/Errors.hpp"
#include "optigrab/util/Process.hpp"

#include <string>

namespace optigrab {
namespace {

// diskutil wants diskN / /dev/diskN, not rdiskN.
std::string toDiskutilPath(std::string path) {
    const std::string raw = "/dev/rdisk";
    if (path.rfind(raw, 0) == 0) {
        return "/dev/disk" + path.substr(raw.size());
    }
    return path;
}

}  // namespace

void MacDriveEjector::eject(const std::string& devicePath) {
    const std::string path = toDiskutilPath(devicePath);
    std::string out;
    std::string err;

    // Prefer diskutil (handles USB optical trays on modern macOS).
    if (runProcess({"diskutil", "eject", path}, out, err) == 0) {
        return;
    }

    // Fallback: drutil (SuperDrive-era; still present on some systems).
    if (runProcess({"drutil", "tray", "eject"}, out, err) == 0) {
        return;
    }

    throw DriveError("eject failed for " + devicePath +
                     (err.empty() ? (out.empty() ? std::string{} : out) : err));
}

}  // namespace optigrab
