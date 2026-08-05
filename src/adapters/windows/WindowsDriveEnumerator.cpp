#include "optigrab/adapters/windows/WindowsDriveEnumerator.hpp"

#ifdef _WIN32

#include "optigrab/adapters/windows/WindowsHeaders.hpp"

#include <string>

namespace optigrab {
namespace {

std::string driveLetterPath(char letter) {
    std::string p;
    p.push_back(letter);
    p.push_back(':');
    return p;
}

std::string queryModel(char letter) {
    // \\.\D:
    const std::string device = std::string("\\\\.\\") + letter + ":";
    HANDLE h = CreateFileA(device.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                           OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        return {};
    }

    STORAGE_PROPERTY_QUERY query{};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;

    BYTE buffer[1024]{};
    DWORD bytes = 0;
    std::string model;
    if (DeviceIoControl(h, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), buffer,
                        sizeof(buffer), &bytes, nullptr)) {
        auto* desc = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(buffer);
        if (desc->ProductIdOffset != 0 && desc->ProductIdOffset < bytes) {
            model = reinterpret_cast<const char*>(buffer + desc->ProductIdOffset);
            while (!model.empty() && (model.back() == ' ' || model.back() == '\0')) {
                model.pop_back();
            }
        }
    }
    CloseHandle(h);
    return model;
}

}  // namespace

std::vector<DriveInfo> WindowsDriveEnumerator::listDrives() {
    std::vector<DriveInfo> drives;
    int index = 0;

    const DWORD mask = GetLogicalDrives();
    for (int i = 0; i < 26; ++i) {
        if ((mask & (1u << i)) == 0) {
            continue;
        }
        const char letter = static_cast<char>('A' + i);
        const std::string root = driveLetterPath(letter) + "\\";
        if (GetDriveTypeA(root.c_str()) != DRIVE_CDROM) {
            continue;
        }
        DriveInfo info;
        info.path = driveLetterPath(letter);
        info.index = index++;
        info.model = queryModel(letter);
        drives.push_back(std::move(info));
    }
    return drives;
}

}  // namespace optigrab

#else

namespace optigrab {

std::vector<DriveInfo> WindowsDriveEnumerator::listDrives() {
    return {};
}

}  // namespace optigrab

#endif
