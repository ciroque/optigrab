#include "optigrab/adapters/linux/LinuxDriveEnumerator.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace optigrab {
namespace {

std::string readModel(const std::string& srName) {
    // /sys/block/sr0/device/model
    const auto path = std::filesystem::path("/sys/block") / srName / "device" / "model";
    std::ifstream in(path);
    if (!in) {
        return {};
    }
    std::string model;
    std::getline(in, model);
    // Trim trailing spaces (sysfs often pads).
    while (!model.empty() && (model.back() == ' ' || model.back() == '\n' || model.back() == '\r')) {
        model.pop_back();
    }
    return model;
}

}  // namespace

std::vector<DriveInfo> LinuxDriveEnumerator::listDrives() {
    std::vector<DriveInfo> drives;
    int index = 0;

    namespace fs = std::filesystem;
    const fs::path dev{"/dev"};
    if (!fs::exists(dev)) {
        return drives;
    }

    // Prefer /dev/sr* (SCSI CD-ROM).
    for (int i = 0; i < 32; ++i) {
        const auto path = dev / ("sr" + std::to_string(i));
        if (!fs::exists(path)) {
            continue;
        }
        DriveInfo info;
        info.path = path.string();
        info.index = index++;
        info.model = readModel("sr" + std::to_string(i));
        drives.push_back(std::move(info));
    }

    // Also pick up /dev/cdrom symlink if it points somewhere not already listed.
    const auto cdrom = dev / "cdrom";
    if (fs::exists(cdrom)) {
        std::error_code ec;
        const auto canonical = fs::weakly_canonical(cdrom, ec).string();
        bool found = false;
        for (const auto& d : drives) {
            if (d.path == canonical || d.path == cdrom.string()) {
                found = true;
                break;
            }
        }
        if (!found) {
            DriveInfo info;
            info.path = cdrom.string();
            info.index = index++;
            info.model = "cdrom";
            drives.push_back(std::move(info));
        }
    }

    return drives;
}

}  // namespace optigrab
