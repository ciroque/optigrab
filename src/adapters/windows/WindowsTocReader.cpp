#include "optigrab/adapters/windows/WindowsTocReader.hpp"

#include "optigrab/domain/Errors.hpp"

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
    // Accept "D:", "D:\", "\\.\D:", "/dev/sr0" (invalid on Win — pass through for error).
    if (devicePath.size() >= 4 && devicePath[0] == '\\' && devicePath[1] == '\\') {
        return devicePath;
    }
    if (devicePath.size() >= 2 && devicePath[1] == ':') {
        const char letter = static_cast<char>(std::toupper(static_cast<unsigned char>(devicePath[0])));
        return std::string("\\\\.\\") + letter + ":";
    }
    return devicePath;
}

// MSF in CDROM_TOC Address is BCD (sometimes) — Windows docs: Address[0]=reserved,
// [1]=M, [2]=S, [3]=F in binary for IOCTL_CDROM_READ_TOC on modern Windows.
std::int64_t msfToLba(const UCHAR address[4]) {
    const int m = static_cast<int>(address[1]);
    const int s = static_cast<int>(address[2]);
    const int f = static_cast<int>(address[3]);
    return static_cast<std::int64_t>(((m * 60) + s) * 75 + f - 150);
}

}  // namespace

DiscInfo WindowsTocReader::readToc(const std::string& devicePath) {
    const std::string ntPath = toNtDevicePath(devicePath);
    HANDLE h = CreateFileA(ntPath.c_str(), GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        const DWORD err = GetLastError();
        throw TocError("Failed to open device for TOC: " + devicePath + " (Win32 " +
                       std::to_string(err) + ")");
    }

    CDROM_TOC toc{};
    DWORD bytes = 0;
    const BOOL ok =
        DeviceIoControl(h, IOCTL_CDROM_READ_TOC, nullptr, 0, &toc, sizeof(toc), &bytes, nullptr);
    CloseHandle(h);

    if (!ok) {
        const DWORD err = GetLastError();
        throw TocError("IOCTL_CDROM_READ_TOC failed on " + devicePath + " (Win32 " +
                       std::to_string(err) + "). Is a disc inserted? Close other apps using the drive.");
    }

    DiscInfo disc;
    disc.devicePath = devicePath;

    const int first = static_cast<int>(toc.FirstTrack);
    const int last = static_cast<int>(toc.LastTrack);
    if (first < 1 || last < first) {
        throw TocError("No tracks found on " + devicePath);
    }

    // TrackData entries: one per track plus lead-out (0xAA).
    const int trackCount = last - first + 1;
    for (int i = 0; i < trackCount; ++i) {
        const auto& td = toc.TrackData[i];
        TrackInfo info;
        info.number = static_cast<int>(td.TrackNumber);
        info.startLba = msfToLba(td.Address);
        // Control bit 0x04 set => data track
        info.audio = (td.Control & 0x04) == 0;

        const auto& next = toc.TrackData[i + 1];
        const auto nextLba = msfToLba(next.Address);
        info.sectors = nextLba > info.startLba ? (nextLba - info.startLba) : 0;
        disc.tracks.push_back(std::move(info));
    }

    if (disc.tracks.empty()) {
        throw TocError("No tracks found on " + devicePath);
    }
    return disc;
}

}  // namespace optigrab

#else

namespace optigrab {

DiscInfo WindowsTocReader::readToc(const std::string& devicePath) {
    throw TocError("WindowsTocReader is not available on this platform (" + devicePath + ")");
}

}  // namespace optigrab

#endif
