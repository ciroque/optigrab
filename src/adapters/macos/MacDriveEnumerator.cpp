#include "optigrab/adapters/macos/MacDriveEnumerator.hpp"

#include "optigrab/util/Process.hpp"

#include <cctype>
#include <regex>
#include <set>
#include <sstream>
#include <string>

namespace optigrab {
namespace {

// Prefer raw device nodes for libcdio / low-level CDDA access.
std::string toRawDevice(std::string path) {
    // /dev/disk2 -> /dev/rdisk2
    const std::string prefix = "/dev/disk";
    if (path.rfind(prefix, 0) == 0) {
        return "/dev/rdisk" + path.substr(prefix.size());
    }
    return path;
}

bool looksLikeWholeDisk(const std::string& id) {
    // disk2, disk12 — not partitions like disk2s1
    if (id.rfind("disk", 0) != 0 || id.size() <= 4) {
        return false;
    }
    for (size_t i = 4; i < id.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(id[i]))) {
            return false;
        }
    }
    return true;
}

std::string diskutilInfoField(const std::string& diskId, const std::string& key) {
    std::string out;
    std::string err;
    if (runProcess({"diskutil", "info", diskId}, out, err) != 0) {
        return {};
    }
    const std::string needle = key + ":";
    std::istringstream iss(out);
    std::string line;
    while (std::getline(iss, line)) {
        const auto pos = line.find(needle);
        if (pos == std::string::npos) {
            continue;
        }
        std::string value = line.substr(pos + needle.size());
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
            value.erase(value.begin());
        }
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
            value.pop_back();
        }
        return value;
    }
    return {};
}

bool isOpticalDisk(const std::string& diskId) {
    const auto protocol = diskutilInfoField(diskId, "Protocol");
    if (protocol.find("Optical") != std::string::npos) {
        return true;
    }
    for (const char* key : {"Content (IOContent)", "Content"}) {
        const auto content = diskutilInfoField(diskId, key);
        if (content.find("CD_") != std::string::npos || content.find("DVD_") != std::string::npos) {
            return true;
        }
    }
    const auto optical = diskutilInfoField(diskId, "Optical Drive");
    if (!optical.empty() && optical != "No" && optical != "no") {
        return true;
    }
    return false;
}

std::string modelForDisk(const std::string& diskId) {
    auto name = diskutilInfoField(diskId, "Device / Media Name");
    if (name.empty()) {
        name = diskutilInfoField(diskId, "Media Name");
    }
    if (name.empty()) {
        name = diskutilInfoField(diskId, "Device Name");
    }
    return name;
}

// Whole-disk ids from `diskutil list` (headers + scheme lines).
std::set<std::string> wholeDisksFromList(const std::string& listOut) {
    std::set<std::string> ids;
    static const std::regex diskRe(R"((disk\d+))");
    std::istringstream iss(listOut);
    std::string line;
    while (std::getline(iss, line)) {
        // Prefer lines that mention optical schemes — always keep those disks.
        const bool opticalScheme =
            line.find("CD_") != std::string::npos || line.find("DVD_") != std::string::npos;
        // Header: /dev/disk2 (external, physical):
        // Body:   0: CD_partition_scheme ... disk2
        std::sregex_iterator it(line.begin(), line.end(), diskRe);
        std::sregex_iterator end;
        for (; it != end; ++it) {
            const auto id = (*it)[1].str();
            if (!looksLikeWholeDisk(id)) {
                continue;
            }
            if (opticalScheme || line.find("/dev/" + id) != std::string::npos) {
                ids.insert(id);
            }
        }
    }
    return ids;
}

}  // namespace

std::vector<DriveInfo> MacDriveEnumerator::listDrives() {
    std::vector<DriveInfo> drives;
    int index = 0;

    std::string listOut;
    std::string listErr;
    if (runProcess({"diskutil", "list"}, listOut, listErr) != 0) {
        return drives;
    }

    // Fast path: disks that already show a CD/DVD scheme in list output.
    std::set<std::string> opticalIds;
    {
        std::istringstream iss(listOut);
        std::string line;
        static const std::regex diskRe(R"((disk\d+))");
        while (std::getline(iss, line)) {
            if (line.find("CD_") == std::string::npos && line.find("DVD_") == std::string::npos) {
                continue;
            }
            std::sregex_iterator it(line.begin(), line.end(), diskRe);
            std::sregex_iterator end;
            for (; it != end; ++it) {
                const auto id = (*it)[1].str();
                if (looksLikeWholeDisk(id)) {
                    opticalIds.insert(id);
                }
            }
        }
    }

    // Also inspect whole disks from the list (empty tray USB drives may not show CD_ yet).
    for (const auto& id : wholeDisksFromList(listOut)) {
        if (opticalIds.count(id) != 0) {
            continue;
        }
        if (isOpticalDisk(id)) {
            opticalIds.insert(id);
        }
    }

    for (const auto& id : opticalIds) {
        DriveInfo info;
        info.path = toRawDevice("/dev/" + id);
        info.index = index++;
        info.model = modelForDisk(id);
        if (info.model.empty()) {
            info.model = id;
        }
        drives.push_back(std::move(info));
    }

    return drives;
}

}  // namespace optigrab
