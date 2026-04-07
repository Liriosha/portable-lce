#ifndef WINAPISTUBS_H
#define WINAPISTUBS_H

#pragma once

#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cwchar>

#define __cdecl
#define _vsnprintf_s vsnprintf;

typedef unsigned int DWORD;
typedef unsigned char BYTE;
typedef BYTE* PBYTE;
typedef int HRESULT;
typedef unsigned int UINT;
typedef void* HANDLE;
typedef int INT;
typedef unsigned int* LPDWORD;
typedef char CHAR;
typedef uintptr_t ULONG_PTR;
typedef long LONG;
typedef unsigned long long PlayerUID;
typedef DWORD WORD;
typedef DWORD* PDWORD;

typedef struct {
    DWORD LowPart;
    LONG HighPart;
    long long QuadPart;
} LARGE_INTEGER;

typedef struct {
    DWORD LowPart;
    LONG HighPart;
    long long QuadPart;
} ULARGE_INTEGER;

typedef long long LONGLONG;
typedef wchar_t *LPWSTR, *PWSTR;
typedef unsigned char boolean;  // java brainrot
#define __debugbreak()
#define CONST const
typedef unsigned long ULONG;
// typedef unsigned char byte;
typedef short SHORT;
typedef float FLOAT;

#define ERROR_SUCCESS 0L
#define ERROR_IO_PENDING 997L  // dderror
#define ERROR_CANCELLED 1223L

#define MEM_COMMIT 0x1000
#define MEM_RESERVE 0x2000
#define MEM_DECOMMIT 0x4000
#define PAGE_READWRITE 0x04

// https://learn.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-filetime
typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

typedef struct _MEMORYSTATUS {
    DWORD dwLength;
    DWORD dwMemoryLoad;
    size_t dwTotalPhys;
    size_t dwAvailPhys;
    size_t dwTotalPageFile;
    size_t dwAvailPageFile;
    size_t dwTotalVirtual;
    size_t dwAvailVirtual;
} MEMORYSTATUS, *LPMEMORYSTATUS;

typedef enum _GET_FILEEX_INFO_LEVELS {
    GetFileExInfoStandard,
    GetFileExMaxInfoLevel
} GET_FILEEX_INFO_LEVELS;

typedef void* XMEMCOMPRESSION_CONTEXT;
typedef void* XMEMDECOMPRESSION_CONTEXT;

// https://learn.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-systemtime
typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

#define TLS_OUT_OF_INDEXES ((DWORD)0xFFFFFFFF)
// https://learn.microsoft.com/en-us/cpp/c-runtime-library/truncate?view=msvc-170
#define _TRUNCATE ((size_t)-1)

#define DECLARE_HANDLE(name) typedef HANDLE name
DECLARE_HANDLE(HINSTANCE);

typedef HINSTANCE HMODULE;

#define _HRESULT_TYPEDEF_(_sc) _sc

#define FAILED(Status) ((HRESULT)(Status) < 0)
#define MAKE_HRESULT(sev, fac, code)                                       \
    ((HRESULT)(((unsigned int)(sev) << 31) | ((unsigned int)(fac) << 16) | \
               ((unsigned int)(code))))
#define MAKE_SCODE(sev, fac, code)                                       \
    ((SCODE)(((unsigned int)(sev) << 31) | ((unsigned int)(fac) << 16) | \
             ((unsigned int)(code))))
#define E_FAIL _HRESULT_TYPEDEF_(0x80004005L)
#define E_ABORT _HRESULT_TYPEDEF_(0x80004004L)
#define E_NOINTERFACE _HRESULT_TYPEDEF_(0x80004002L)

// https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-globalmemorystatus
static inline void GlobalMemoryStatus(LPMEMORYSTATUS lpBuffer) {
    // TODO: Parse /proc/meminfo and set lpBuffer based on that. Probably will
    // also need another different codepath for macOS too.
}

static inline DWORD GetLastError(void) { return errno; }

#ifdef __LP64__
static inline int64_t InterlockedCompareExchangeRelease64(
    int64_t volatile* Destination, int64_t Exchange, int64_t Comperand) {
    int64_t expected = Comperand;
    __atomic_compare_exchange_n(Destination, &expected, Exchange, false,
                                __ATOMIC_RELEASE, __ATOMIC_RELAXED);
    return expected;
}
#else
static inline int64_t InterlockedCompareExchangeRelease(
    LONG volatile* Destination, LONG Exchange, LONG Comperand) {
    LONG expected = Comperand;
    __atomic_compare_exchange_n(Destination, &expected, Exchange, false,
                                __ATOMIC_RELEASE, __ATOMIC_RELAXED);
    return expected;
}
#endif

// internal helper: convert time_t to FILETIME (100ns intervals since
// 1601-01-01)
static inline FILETIME _TimeToFileTime(time_t t) {
    const uint64_t EPOCH_DIFF = 11644473600ULL;
    uint64_t val = ((uint64_t)t + EPOCH_DIFF) * 10000000ULL;
    FILETIME ft;
    ft.dwLowDateTime = (DWORD)(val & 0xFFFFFFFF);
    ft.dwHighDateTime = (DWORD)(val >> 32);
    return ft;
}

// internal helper: convert FILETIME (100ns since 1601) to time_t (seconds since
// 1970)
static inline time_t _FileTimeToTimeT(const FILETIME& ft) {
    uint64_t val = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    const uint64_t EPOCH_DIFF =
        116444736000000000ULL;  // 100ns intervals between 1601-01-01 and
                                // 1970-01-01
    return (time_t)((val - EPOCH_DIFF) / 10000000ULL);
}

// internal helper: read the current wall clock into a timespec
static inline void _CurrentTimeSpec(struct timespec* ts) {
#ifdef CLOCK_REALTIME
    clock_gettime(CLOCK_REALTIME, ts);
#else
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;
#endif
}

// internal helper: fill SYSTEMTIME from a broken-down tm + nanosecond remainder
static inline void _FillSystemTime(const struct tm* tm, long tv_nsec,
                                   LPSYSTEMTIME lpSystemTime) {
    lpSystemTime->wYear = tm->tm_year + 1900;
    lpSystemTime->wMonth = tm->tm_mon + 1;
    lpSystemTime->wDayOfWeek = tm->tm_wday;  // 0 = Sunday
    lpSystemTime->wDay = tm->tm_mday;
    lpSystemTime->wHour = tm->tm_hour;
    lpSystemTime->wMinute = tm->tm_min;
    lpSystemTime->wSecond = tm->tm_sec;
    lpSystemTime->wMilliseconds = (WORD)(tv_nsec / 1000000);  // ns to ms
}

// https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getlocaltime
static inline void GetLocalTime(LPSYSTEMTIME lpSystemTime) {
    struct timespec ts;
    _CurrentTimeSpec(&ts);
    struct tm tm;
    localtime_r(&ts.tv_sec, &tm);  // local time
    _FillSystemTime(&tm, ts.tv_nsec, lpSystemTime);
}

// https://learn.microsoft.com/en-us/windows/win32/api/timezoneapi/nf-timezoneapi-filetimetosystemtime
static inline bool FileTimeToSystemTime(const FILETIME* lpFileTime,
                                        LPSYSTEMTIME lpSystemTime) {
    uint64_t ft = ((uint64_t)lpFileTime->dwHighDateTime << 32) |
                  lpFileTime->dwLowDateTime;
    time_t t = _FileTimeToTimeT(*lpFileTime);
    long remainder_ns = (long)((ft % 10000000ULL) * 100);

    struct tm tm;
    gmtime_r(&t, &tm);  // UTC
    _FillSystemTime(&tm, remainder_ns, lpSystemTime);
    return true;
}

// https://learn.microsoft.com/en-us/windows/win32/api/debugapi/nf-debugapi-outputdebugstringa
static inline void OutputDebugStringA(const char* lpOutputString) {
    if (!lpOutputString) return;
    fputs(lpOutputString, stderr);
}

// https://learn.microsoft.com/en-us/windows/win32/api/debugapi/nf-debugapi-outputdebugstringw
static inline void OutputDebugStringW(const wchar_t* lpOutputString) {
    if (!lpOutputString) return;
    fprintf(stderr, "%ls", lpOutputString);
}

static inline void OutputDebugString(const char* lpOutputString) {
    return OutputDebugStringA(lpOutputString);
}

static inline HMODULE GetModuleHandle(const char* lpModuleName) { return 0; }

static inline void* VirtualAlloc(void* lpAddress, size_t dwSize,
                                 DWORD flAllocationType, DWORD flProtect) {
    // MEM_COMMIT | MEM_RESERVE → mmap anonymous
    int prot = 0;
    if (flProtect == 0x04 /*PAGE_READWRITE*/)
        prot = PROT_READ | PROT_WRITE;
    else if (flProtect == 0x40 /*PAGE_EXECUTE_READWRITE*/)
        prot = PROT_READ | PROT_WRITE | PROT_EXEC;
    else if (flProtect == 0x02 /*PAGE_READONLY*/)
        prot = PROT_READ;
    else
        prot = PROT_READ | PROT_WRITE;  // default

    int flags = MAP_PRIVATE | MAP_ANONYMOUS;
    if (lpAddress != nullptr) flags |= MAP_FIXED;

    void* p = mmap(lpAddress, dwSize, prot, flags, -1, 0);
    if (p == MAP_FAILED) return nullptr;
    return p;
}

static inline bool VirtualFree(void* lpAddress, size_t dwSize,
                               DWORD dwFreeType) {
    if (lpAddress == nullptr) return false;
    // MEM_RELEASE (0x8000) frees the whole region
    if (dwFreeType == 0x8000 /*MEM_RELEASE*/) {
        // dwSize should be 0 for MEM_RELEASE per Win32 API, but we don't track
        // allocation sizes Use dwSize if provided, otherwise this is a
        // best-effort
        if (dwSize == 0) dwSize = 4096;  // minimum page
        munmap(lpAddress, dwSize);
    } else {
        // MEM_DECOMMIT (0x4000) - just decommit (make inaccessible)
        madvise(lpAddress, dwSize, MADV_DONTNEED);
    }
    return true;
}

#endif  // WINAPISTUBS_H
