#pragma once

// Centralize Windows SDK includes for MSVC CI (order matters).
// WIN32_LEAN_AND_MEAN is fine if we pull winioctl + ntdd* explicitly.

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <winioctl.h>

// Storage property query (drive model)
#include <ntddstor.h>

// Optical TOC / eject IOCTLs
#include <ntddcdrm.h>

// Some SDKs leave these only in older names; provide fallbacks.
#ifndef ERROR_PATH_NOT_READY
#define ERROR_PATH_NOT_READY 151u  // not always defined; rare path error
#endif
#ifndef ERROR_NOT_READY
#define ERROR_NOT_READY 21u
#endif
#ifndef ERROR_BUSY
#define ERROR_BUSY 170u
#endif

#endif  // _WIN32
