#pragma once

// Centralize Windows SDK includes for MSVC CI (order matters).
// User-mode storage IOCTLs: winioctl.h. Optical TOC: ntddcdrm.h.

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <winioctl.h>

// Prefer SDK headers when present (MSVC / Windows SDK).
#if defined(__has_include)
#  if __has_include(<ntddcdrm.h>)
#    include <ntddcdrm.h>
#  endif
#  if __has_include(<ntddstor.h>)
#    include <ntddstor.h>
#  endif
#else
// Older compilers: include and hope the SDK is complete.
#  include <ntddcdrm.h>
#endif

// ---------------------------------------------------------------------------
// Fallbacks when lean/old SDKs omit symbols. Gated on IOCTL macros (real #defines);
// do not gate on typedef names — those are not preprocessor macros.
// Values match current Windows SDK.
// ---------------------------------------------------------------------------

#ifndef ERROR_NOT_READY
#define ERROR_NOT_READY 21u
#endif
#ifndef ERROR_BUSY
#define ERROR_BUSY 170u
#endif

#ifndef IOCTL_STORAGE_BASE
#define IOCTL_STORAGE_BASE FILE_DEVICE_MASS_STORAGE
#endif

#ifndef IOCTL_STORAGE_QUERY_PROPERTY
#define IOCTL_STORAGE_QUERY_PROPERTY \
    CTL_CODE(IOCTL_STORAGE_BASE, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)

enum STORAGE_PROPERTY_ID_OPTIGRAB {
    StorageDeviceProperty = 0,
    StorageAdapterProperty = 1
};

enum STORAGE_QUERY_TYPE_OPTIGRAB {
    PropertyStandardQuery = 0,
    PropertyExistsQuery = 1
};

typedef struct _STORAGE_PROPERTY_QUERY {
    DWORD PropertyId;
    DWORD QueryType;
    BYTE AdditionalParameters[1];
} STORAGE_PROPERTY_QUERY, *PSTORAGE_PROPERTY_QUERY;

typedef struct _STORAGE_DEVICE_DESCRIPTOR {
    DWORD Version;
    DWORD Size;
    BYTE DeviceType;
    BYTE DeviceTypeModifier;
    BOOLEAN RemovableMedia;
    BOOLEAN CommandQueueing;
    DWORD VendorIdOffset;
    DWORD ProductIdOffset;
    DWORD ProductRevisionOffset;
    DWORD SerialNumberOffset;
    DWORD BusType;
    DWORD RawPropertiesLength;
    BYTE RawDeviceProperties[1];
} STORAGE_DEVICE_DESCRIPTOR, *PSTORAGE_DEVICE_DESCRIPTOR;
#endif  // IOCTL_STORAGE_QUERY_PROPERTY

#ifndef IOCTL_STORAGE_EJECT_MEDIA
#define IOCTL_STORAGE_EJECT_MEDIA \
    CTL_CODE(IOCTL_STORAGE_BASE, 0x0202, METHOD_BUFFERED, FILE_READ_ACCESS)
#endif
#ifndef IOCTL_STORAGE_MEDIA_REMOVAL
#define IOCTL_STORAGE_MEDIA_REMOVAL \
    CTL_CODE(IOCTL_STORAGE_BASE, 0x0201, METHOD_BUFFERED, FILE_READ_ACCESS)

typedef struct _PREVENT_MEDIA_REMOVAL {
    BOOLEAN PreventMediaRemoval;
} PREVENT_MEDIA_REMOVAL, *PPREVENT_MEDIA_REMOVAL;
#endif

#ifndef MAXIMUM_NUMBER_TRACKS
#define MAXIMUM_NUMBER_TRACKS 100
#endif

#ifndef IOCTL_CDROM_BASE
#define IOCTL_CDROM_BASE FILE_DEVICE_CD_ROM
#endif

#ifndef IOCTL_CDROM_READ_TOC
#define IOCTL_CDROM_READ_TOC \
    CTL_CODE(IOCTL_CDROM_BASE, 0x0000, METHOD_BUFFERED, FILE_READ_ACCESS)

typedef struct _TRACK_DATA {
    UCHAR Reserved;
    UCHAR Control : 4;
    UCHAR Adr : 4;
    UCHAR TrackNumber;
    UCHAR Reserved1;
    UCHAR Address[4];
} TRACK_DATA, *PTRACK_DATA;

typedef struct _CDROM_TOC {
    UCHAR Length[2];
    UCHAR FirstTrack;
    UCHAR LastTrack;
    TRACK_DATA TrackData[MAXIMUM_NUMBER_TRACKS];
} CDROM_TOC, *PCDROM_TOC;
#endif  // IOCTL_CDROM_READ_TOC

#endif  // _WIN32
