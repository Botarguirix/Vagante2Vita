/* kernel32 for vagante.exe: wide-character (UTF-16) variants, locale, newer sync APIs.
 * Included by kernel32.c, so it shares open_read_only, file_seek/file_tell and mkdir_host. */
#include <dirent.h>
#include <sched.h>
#include <unistd.h>

/* Guest UTF-16 string -> host UTF-8 (truncated to size). */
static const char *w2u(uint32_t s, char *out, int size) {
    int n = s ? utf16_to_mb(65001, (const uint16_t *)G(s), -1, (uint8_t *)out, size - 1, NULL) : 0;
    out[n > 0 ? n : 0] = 0;
    return out;
}
/* Host UTF-8 -> guest UTF-16 buffer of cap characters; returns the length without the
   terminator, or the required size (with it) when cap is too small, as the W APIs do. */
static uint32_t u2w(const char *s, uint32_t dst, uint32_t cap) {
    int need = mb_to_utf16(65001, (const uint8_t *)s, -1, NULL, 0);
    if (!dst || (uint32_t)need > cap)
        return (uint32_t)need;
    mb_to_utf16(65001, (const uint8_t *)s, -1, (uint16_t *)G(dst), (int)cap);
    return (uint32_t)need - 1;
}
static void host_path_w(uint32_t s, char *out, size_t size) {
    char name[1024];
    path_to_host(w2u(s, name, sizeof name), out, size);
}

/* -- process, system ----------------------------------------------------------------------- */
API(IsProcessorFeaturePresent) {
    uint32_t f = ARG(c, 0); /* SSE (6) and SSE2 (10): the game is built for them */
    ret_stdcall(c, f == 6 || f == 10, 1);
}
API(GetCurrentThread) { ret_stdcall(c, 0xFFFFFFFEu, 0); }
API(GetThreadId) { ret_stdcall(c, rd32(c->fs_base + 0x24), 1); }
API(GetTickCount64) {
    uint64_t ms = platform_ticks_us() / 1000u;
    c->edx.d = (uint32_t)(ms >> 32);
    ret_stdcall(c, (uint32_t)ms, 0);
}
API(QueryPerformanceFrequency) { wr64(ARG(c, 0), 1000000u); ret_stdcall(c, 1, 1); }
API(SleepEx) { platform_sleep_ms(ARG(c, 0) == 0xFFFFFFFFu ? 1 : ARG(c, 0)); ret_stdcall(c, 0, 2); }
API(EncodePointer) { ret_stdcall(c, ARG(c, 0), 1); }
API(DecodePointer) { ret_stdcall(c, ARG(c, 0), 1); }
API(GetVersion) { ret_stdcall(c, 0x23F00206u, 0); } /* 6.2 build 9200 (Windows 8, manifest-less) */
API(VerSetConditionMask) { /* (ULONGLONG mask, DWORD type, BYTE condition): mask unchanged */
    c->edx.d = ARG(c, 1);
    ret_stdcall(c, ARG(c, 0), 4);
}
API(VerifyVersionInfoA) { ret_stdcall(c, 1, 4); } /* (info, type, ULONGLONG mask): a newer system */
API(GetNativeSystemInfo) {
    uint32_t p = ARG(c, 0);
    memset(G(p), 0, 36);
    wr32(p + 4, 4096);                 /* dwPageSize */
    wr32(p + 8, 0x00010000u);          /* lpMinimumApplicationAddress */
    wr32(p + 12, 0x7FFEFFFFu);         /* lpMaximumApplicationAddress */
    wr32(p + 16, 0xF);                 /* active processor mask */
    wr32(p + 20, 4);                   /* dwNumberOfProcessors */
    wr32(p + 24, 586);                 /* dwProcessorType */
    wr32(p + 28, 0x00010000u);         /* dwAllocationGranularity */
    wr16(p + 32, 6);                   /* wProcessorLevel */
    ret_stdcall(c, 0, 1);
}
API(GetStartupInfoW) {
    memset(G(ARG(c, 0)), 0, 68);
    wr32(ARG(c, 0), 68);
    ret_stdcall(c, 0, 1);
}
static uint32_t command_line_w;
API(GetCommandLineW) {
    if (!command_line_w) {
        uint32_t n = u2w(guest_exe_path, 0, 0);
        command_line_w = heap_alloc(2 * n, 1);
        u2w(guest_exe_path, command_line_w, n);
    }
    ret_stdcall(c, command_line_w, 0);
}
API(GetModuleFileNameW) {
    uint32_t buf = ARG(c, 1), n = ARG(c, 2);
    uint32_t len = u2w(guest_exe_path, 0, 0) - 1;
    if (n > len) u2w(guest_exe_path, buf, n);
    ret_stdcall(c, len < n ? len : n, 3);
}
static uint32_t module_by_name(const char *s) {
    return !s ? guest_image_base
         : !_strnicmp(s, "kernel32", 8) ? FAKE_MODULE_KERNEL32
         : !_strnicmp(s, "user32", 6) || !_strnicmp(s, "opengl32", 8) ? FAKE_MODULE_OTHER
         : 0;
}
API(GetModuleHandleW) {
    char name[256];
    uint32_t h = module_by_name(ARG(c, 0) ? w2u(ARG(c, 0), name, sizeof name) : NULL);
    trace("GetModuleHandleW(%s) = %08x", ARG(c, 0) ? name : "NULL", h);
    ret_stdcall(c, h, 1);
}
API(GetModuleHandleExW) {
    char name[256];
    uint32_t flags = ARG(c, 0), h;
    if (flags & 4) /* GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS */
        h = guest_image_base;
    else
        h = module_by_name(ARG(c, 1) ? w2u(ARG(c, 1), name, sizeof name) : NULL);
    if (ARG(c, 2)) wr32(ARG(c, 2), h);
    ret_stdcall(c, h != 0, 3);
}
API(LoadLibraryW) {
    char name[256];
    trace("LoadLibraryW(%s)", w2u(ARG(c, 0), name, sizeof name));
    ret_stdcall(c, FAKE_MODULE_OTHER, 1);
}
API(LoadLibraryExW) {
    char name[256];
    trace("LoadLibraryExW(%s)", w2u(ARG(c, 0), name, sizeof name));
    ret_stdcall(c, FAKE_MODULE_OTHER, 3);
}
API(FreeLibraryAndExitThread) { host_exit(c, ARG(c, 1), "FreeLibraryAndExitThread"); }
API(ExitThread) { host_exit(c, ARG(c, 0), "ExitThread"); }
API(GetEnvironmentVariableA) { last_error = 203; ret_stdcall(c, 0, 3); } /* ERROR_ENVVAR_NOT_FOUND */
API(GetEnvironmentVariableW) { last_error = 203; ret_stdcall(c, 0, 3); }
API(SetEnvironmentVariableW) { ret_stdcall(c, 1, 2); }
API(ExpandEnvironmentStringsA) {
    uint32_t n = (uint32_t)strlen(STR(ARG(c, 0))) + 1;
    if (ARG(c, 1) && ARG(c, 2) >= n) memcpy(G(ARG(c, 1)), G(ARG(c, 0)), n);
    ret_stdcall(c, n, 3);
}
API(LocalFree) { ret_stdcall(c, 0, 1); }
API(GetConsoleWindow) { ret_stdcall(c, 0, 0); }
API(PeekNamedPipe) { ret_stdcall(c, 0, 6); }
API(ReadConsoleW) { ret_stdcall(c, 0, 5); }
API(WriteConsoleW) {
    char text[2048];
    uint32_t n = ARG(c, 2);
    int k = utf16_to_mb(65001, (const uint16_t *)G(ARG(c, 1)), (int)n, (uint8_t *)text, sizeof text - 1, NULL);
    text[k > 0 ? k : 0] = 0;
    trace("WriteConsoleW: %s", text);
    if (ARG(c, 3)) wr32(ARG(c, 3), n);
    ret_stdcall(c, 1, 5);
}
API(ReadProcessMemory) { ret_stdcall(c, 0, 5); }
API(DeviceIoControl) { ret_stdcall(c, 0, 8); }
API(GetThreadContext) { ret_stdcall(c, 0, 2); }
API(SuspendThread) { ret_stdcall(c, 0, 1); }
API(ResumeThread) { ret_stdcall(c, 1, 1); }
API(SetThreadStackGuarantee) { ret_stdcall(c, 1, 1); }
API(SetThreadExecutionState) { ret_stdcall(c, 1, 1); }
API(InitializeCriticalSectionEx) { ret_stdcall(c, 1, 3); }
API(WaitForSingleObjectEx) { ret_stdcall(c, 0, 3); }
API(WaitForMultipleObjects) { ret_stdcall(c, 0, 4); }
API(CreateEventW) { ret_stdcall(c, handle_new(H_EVENT, NULL, ARG(c, 2)), 4); }
API(ResetEvent) { ret_stdcall(c, 1, 1); }

/* SRW locks: the lock word is 0 (free) or 1 (held); exclusive mode only. */
API(InitializeSRWLock) { wr32(ARG(c, 0), 0); ret_stdcall(c, 0, 1); }
API(AcquireSRWLockExclusive) {
    uint32_t *w = (uint32_t *)G(ARG(c, 0)), expected = 0;
    while (!__atomic_compare_exchange_n(w, &expected, 1, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
        expected = 0;
        sched_yield();
    }
    ret_stdcall(c, 0, 1);
}
API(ReleaseSRWLockExclusive) {
    __atomic_store_n((uint32_t *)G(ARG(c, 0)), 0, __ATOMIC_RELEASE);
    ret_stdcall(c, 0, 1);
}
/* Interlocked singly linked list: head is {Next, Depth+Sequence} (8 bytes). */
API(InitializeSListHead) { wr64(ARG(c, 0), 0); ret_stdcall(c, 0, 1); }
API(InterlockedPushEntrySList) {
    static pthread_mutex_t slist = PTHREAD_MUTEX_INITIALIZER;
    uint32_t head = ARG(c, 0), entry = ARG(c, 1);
    pthread_mutex_lock(&slist);
    uint32_t old = rd32(head);
    wr32(entry, old);
    wr32(head, entry);
    wr16(head + 4, (uint16_t)(rd16(head + 4) + 1));
    pthread_mutex_unlock(&slist);
    ret_stdcall(c, old, 2);
}

/* -- locale: en-US ------------------------------------------------------------------------- */
static const char *locale_value(uint32_t type) {
    switch (type & 0xFFFF) {
    case 0x0001: return "0409";          /* LOCALE_ILANGUAGE */
    case 0x0003: return "ENU";           /* LOCALE_SABBREVLANGNAME */
    case 0x0007: return "USA";           /* LOCALE_SABBREVCTRYNAME */
    case 0x000B: return "437";           /* LOCALE_IDEFAULTCODEPAGE */
    case 0x000E: return ".";             /* LOCALE_SDECIMAL */
    case 0x000F: return ",";             /* LOCALE_STHOUSAND */
    case 0x0010: return "3;0";           /* LOCALE_SGROUPING */
    case 0x0014: return "$";             /* LOCALE_SCURRENCY */
    case 0x0015: return "USD";           /* LOCALE_SINTLSYMBOL */
    case 0x0016: return ".";             /* LOCALE_SMONDECIMALSEP */
    case 0x0017: return ",";             /* LOCALE_SMONTHOUSANDSEP */
    case 0x0018: return "3;0";           /* LOCALE_SMONGROUPING */
    case 0x0019: return "2";             /* LOCALE_ICURRDIGITS */
    case 0x0050: return "";              /* LOCALE_SPOSITIVESIGN */
    case 0x0051: return "-";             /* LOCALE_SNEGATIVESIGN */
    case 0x0059: return "en";            /* LOCALE_SISO639LANGNAME */
    case 0x005A: return "US";            /* LOCALE_SISO3166CTRYNAME */
    case 0x005C: return "en-US";         /* LOCALE_SNAME */
    case 0x1001: return "English";       /* LOCALE_SENGLISHLANGUAGENAME */
    case 0x1002: return "United States"; /* LOCALE_SENGLISHCOUNTRYNAME */
    case 0x1004: return "1252";          /* LOCALE_IDEFAULTANSICODEPAGE */
    default: return "";
    }
}
static void locale_info(Cpu *c, uint32_t type, uint32_t buf, uint32_t cap, int wide, unsigned nargs) {
    const char *v = locale_value(type);
    if (type & 0x20000000u) { /* LOCALE_RETURN_NUMBER */
        if (buf && cap >= (wide ? 2u : 4u))
            wr32(buf, (uint32_t)strtoul(v, NULL, (type & 0xFFFF) == 1 ? 16 : 10));
        ret_stdcall(c, wide ? 2 : 4, nargs);
        return;
    }
    uint32_t n = (uint32_t)strlen(v) + 1;
    if (cap) {
        if (cap < n) { last_error = 122; ret_stdcall(c, 0, nargs); return; }
        if (wide) u2w(v, buf, cap);
        else memcpy(G(buf), v, n);
    }
    ret_stdcall(c, n, nargs);
}
API(GetLocaleInfoA) { locale_info(c, ARG(c, 1), ARG(c, 2), ARG(c, 3), 0, 4); }
API(GetLocaleInfoW) { locale_info(c, ARG(c, 1), ARG(c, 2), ARG(c, 3), 1, 4); }
API(GetLocaleInfoEx) { locale_info(c, ARG(c, 1), ARG(c, 2), ARG(c, 3), 1, 4); }
API(IsValidLocale) { ret_stdcall(c, 1, 2); }
API(EnumSystemLocalesA) { ret_stdcall(c, 1, 2); }
API(EnumSystemLocalesW) { ret_stdcall(c, 1, 2); }
API(GetUserGeoID) { ret_stdcall(c, 244 /* United States */, 1); }
API(GetGeoInfoA) { ret_stdcall(c, 0, 5); }
API(IsDBCSLeadByteEx) { ret_stdcall(c, 0, 2); }
API(GetDateFormatW) { ret_stdcall(c, 0, 6); }
API(GetTimeFormatW) { ret_stdcall(c, 0, 6); }
/* GetTimeFormatA(locale, flags, time, format, buffer, cap): the game's log timestamps
   (NowTime) ask for "HH':'mm':'ss" of the current local time. */
API(GetTimeFormatA) {
    time_t now = time(NULL);
    struct tm tm;
#ifdef _WIN32
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    char text[16];
    int n = snprintf(text, sizeof text, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec) + 1;
    uint32_t buf = ARG(c, 4), cap = ARG(c, 5);
    if (cap && (uint32_t)n > cap) { last_error = 122; ret_stdcall(c, 0, 6); return; }
    if (cap) memcpy(G(buf), text, (size_t)n);
    ret_stdcall(c, (uint32_t)n, 6);
}
API(GetCurrencyFormatW) { ret_stdcall(c, 0, 6); }
API(FormatMessageA) { ret_stdcall(c, 0, 7); }
API(FormatMessageW) { ret_stdcall(c, 0, 7); }
/* LCMapStringEx(name, flags, src, len, dst, cap, ...): ASCII case mapping, otherwise identity. */
API(LCMapStringEx) {
    uint32_t src = ARG(c, 2), dst = ARG(c, 4), cap = ARG(c, 5), flags = ARG(c, 1);
    int32_t len = (int32_t)ARG(c, 3);
    if (len < 0) { for (len = 0; rd16(src + 2 * len); len++) {} len++; }
    if (cap) {
        if ((uint32_t)len > cap) { ret_stdcall(c, 0, 9); return; }
        for (int32_t i = 0; i < len; i++) {
            uint16_t ch = rd16(src + 2 * i);
            if ((flags & 0x100) && ch >= 'A' && ch <= 'Z') ch += 32;  /* LCMAP_LOWERCASE */
            if ((flags & 0x200) && ch >= 'a' && ch <= 'z') ch -= 32;  /* LCMAP_UPPERCASE */
            wr16(dst + 2 * i, ch);
        }
    }
    ret_stdcall(c, (uint32_t)len, 9);
}
API(FoldStringW) {
    uint32_t src = ARG(c, 1), dst = ARG(c, 3), cap = ARG(c, 4);
    int32_t len = (int32_t)ARG(c, 2);
    if (len < 0) { for (len = 0; rd16(src + 2 * len); len++) {} len++; }
    if (cap && (uint32_t)len <= cap) memcpy(G(dst), G(src), 2u * (uint32_t)len);
    ret_stdcall(c, cap && (uint32_t)len > cap ? 0 : (uint32_t)len, 5);
}
static int compare_w(uint32_t a, int32_t la, uint32_t b, int32_t lb, int nocase) {
    if (la < 0) for (la = 0; rd16(a + 2 * la); la++) {}
    if (lb < 0) for (lb = 0; rd16(b + 2 * lb); lb++) {}
    for (int32_t i = 0; i < la && i < lb; i++) {
        uint16_t x = rd16(a + 2 * i), y = rd16(b + 2 * i);
        if (nocase) { if (x >= 'A' && x <= 'Z') x += 32; if (y >= 'A' && y <= 'Z') y += 32; }
        if (x != y) return x < y ? 1 : 3;
    }
    return la == lb ? 2 : la < lb ? 1 : 3; /* CSTR_LESS_THAN / EQUAL / GREATER_THAN */
}
API(CompareStringW) { /* (lcid, flags, a, la, b, lb) */
    ret_stdcall(c, compare_w(ARG(c, 2), (int32_t)ARG(c, 3), ARG(c, 4), (int32_t)ARG(c, 5), ARG(c, 1) & 1), 6);
}
API(CompareStringEx) { /* (name, flags, a, la, b, lb, version, reserved, param) */
    ret_stdcall(c, compare_w(ARG(c, 2), (int32_t)ARG(c, 3), ARG(c, 4), (int32_t)ARG(c, 5), ARG(c, 1) & 1), 9);
}

/* -- time ---------------------------------------------------------------------------------- */
static void unix_to_systemtime(time_t t, uint32_t st, unsigned ms) {
    struct tm tm;
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    wr16(st + 0, (uint16_t)(tm.tm_year + 1900)); wr16(st + 2, (uint16_t)(tm.tm_mon + 1));
    wr16(st + 4, (uint16_t)tm.tm_wday); wr16(st + 6, (uint16_t)tm.tm_mday);
    wr16(st + 8, (uint16_t)tm.tm_hour); wr16(st + 10, (uint16_t)tm.tm_min);
    wr16(st + 12, (uint16_t)tm.tm_sec); wr16(st + 14, (uint16_t)ms);
}
API(GetSystemTime) { unix_to_systemtime(time(NULL), ARG(c, 0), 0); ret_stdcall(c, 0, 1); }
API(FileTimeToSystemTime) {
    uint64_t ft = rd64(ARG(c, 0));
    unix_to_systemtime((time_t)(ft / 10000000ull - 11644473600ull), ARG(c, 1), (unsigned)(ft / 10000u % 1000u));
    ret_stdcall(c, 1, 2);
}
API(SystemTimeToTzSpecificLocalTime) { memcpy(G(ARG(c, 2)), G(ARG(c, 1)), 16); ret_stdcall(c, 1, 3); }

/* -- files (wide) -------------------------------------------------------------------------- */
API(CreateFileW) {
    char path[1200], name[1024];
    w2u(ARG(c, 0), name, sizeof name);
    path_to_host(name, path, sizeof path);
    uint32_t access = ARG(c, 1), disposition = ARG(c, 4);
    int write = (access & 0x40000000u) != 0;
    FILE *f = NULL;
    if (!write) {
        f = open_read_only(path);
    } else if (disposition == 3) {          /* OPEN_EXISTING */
        f = fopen(path, "r+b");
    } else if (disposition == 4) {          /* OPEN_ALWAYS */
        if (!(f = fopen(path, "r+b"))) f = fopen(path, "w+b");
    } else if (disposition == 1) {          /* CREATE_NEW */
        if ((f = fopen(path, "rb"))) { fclose(f); f = NULL; } else f = fopen(path, "w+b");
    } else {                                /* CREATE_ALWAYS, TRUNCATE_EXISTING */
        f = fopen(path, "w+b");
    }
    uint32_t h = f ? handle_new(H_FILE, f, 0) : 0xFFFFFFFFu;
    last_error = f ? 0 : 2;
    trace("CreateFileW(%s, access=%08x, disposition=%u) = %08x", name, access, disposition, h);
    ret_stdcall(c, h, 7);
}
API(GetFileAttributesW) {
    char path[1200];
    struct stat st;
    host_path_w(ARG(c, 0), path, sizeof path);
    uint32_t r = stat(path, &st) ? 0xFFFFFFFFu : (st.st_mode & S_IFDIR) ? 0x10 : 0x80;
    if (r == 0xFFFFFFFFu) last_error = 2;
    ret_stdcall(c, r, 1);
}
API(CreateDirectoryW) {
    char path[1200];
    host_path_w(ARG(c, 0), path, sizeof path);
    int ok = mkdir_host(path) == 0;
    if (!ok) last_error = 183; /* ERROR_ALREADY_EXISTS */
    ret_stdcall(c, ok, 2);
}
API(RemoveDirectoryW) {
    char path[1200];
    host_path_w(ARG(c, 0), path, sizeof path);
    ret_stdcall(c, rmdir(path) == 0, 1);
}
API(DeleteFileW) {
    char path[1200];
    host_path_w(ARG(c, 0), path, sizeof path);
    ret_stdcall(c, remove(path) == 0, 1);
}
API(MoveFileExW) {
    char from[1200], to[1200];
    host_path_w(ARG(c, 0), from, sizeof from);
    host_path_w(ARG(c, 1), to, sizeof to);
    if (ARG(c, 2) & 1) remove(to); /* MOVEFILE_REPLACE_EXISTING */
    ret_stdcall(c, rename(from, to) == 0, 3);
}
API(CopyFileW) {
    char from[1200], to[1200];
    host_path_w(ARG(c, 0), from, sizeof from);
    host_path_w(ARG(c, 1), to, sizeof to);
    FILE *a = fopen(from, "rb"), *b = a ? fopen(to, "wb") : NULL;
    char buf[8192];
    size_t n;
    while (a && b && (n = fread(buf, 1, sizeof buf, a)) > 0) fwrite(buf, 1, n, b);
    if (a) fclose(a);
    if (b) fclose(b);
    ret_stdcall(c, a && b, 3);
}
API(GetFullPathNameW) { /* (name, cap, buffer, filepart): relative names resolve under C:\Vagante */
    char name[1024], full[1100];
    w2u(ARG(c, 0), name, sizeof name);
    if (name[0] && name[1] == ':') snprintf(full, sizeof full, "%s", name);
    else snprintf(full, sizeof full, "C:\\Vagante\\%s", name);
    uint32_t r = u2w(full, ARG(c, 2), ARG(c, 1));
    if (ARG(c, 3)) wr32(ARG(c, 3), 0);
    ret_stdcall(c, r, 4);
}
API(GetCurrentDirectoryW) { ret_stdcall(c, u2w("C:\\Vagante", ARG(c, 1), ARG(c, 0)), 2); }
API(GetWindowsDirectoryW) { ret_stdcall(c, u2w("C:\\Windows", ARG(c, 0), ARG(c, 1)), 2); }
API(GetSystemDirectoryA) {
    const char *d = "C:\\Windows\\system32";
    uint32_t n = (uint32_t)strlen(d);
    if (ARG(c, 1) > n) strcpy(STR(ARG(c, 0)), d);
    ret_stdcall(c, ARG(c, 1) > n ? n : n + 1, 2);
}
API(GetDriveTypeW) { ret_stdcall(c, 3 /* DRIVE_FIXED */, 1); }
API(AreFileApisANSI) { ret_stdcall(c, 1, 0); }
static int64_t file_size_of(FILE *f) {
    int64_t here = file_tell(f);
    file_seek(f, 0, SEEK_END);
    int64_t size = file_tell(f);
    file_seek(f, here, SEEK_SET);
    return size;
}
API(GetFileSizeEx) {
    FILE *f = handle_get(ARG(c, 0), H_FILE);
    if (f) wr64(ARG(c, 1), (uint64_t)file_size_of(f));
    ret_stdcall(c, f != NULL, 2);
}
API(SetFilePointerEx) { /* (handle, LARGE_INTEGER distance (2 slots), new pointer, method) */
    FILE *f = handle_get(ARG(c, 0), H_FILE);
    int64_t distance = (int64_t)(((uint64_t)ARG(c, 2) << 32) | ARG(c, 1));
    if (!f || file_seek(f, distance, (int)ARG(c, 4))) { ret_stdcall(c, 0, 5); return; }
    if (ARG(c, 3)) wr64(ARG(c, 3), (uint64_t)file_tell(f));
    ret_stdcall(c, 1, 5);
}
API(SetEndOfFile) { ret_stdcall(c, 1, 1); }
API(GetFileInformationByHandle) {
    FILE *f = handle_get(ARG(c, 0), H_FILE);
    uint32_t p = ARG(c, 1);
    memset(G(p), 0, 52);
    if (f) {
        int64_t size = file_size_of(f);
        wr32(p, 0x80);
        wr32(p + 32, (uint32_t)((uint64_t)size >> 32));
        wr32(p + 36, (uint32_t)size);
        wr32(p + 40, 1);
    }
    ret_stdcall(c, f != NULL, 2);
}

/* FindFirstFileW/ExW / FindNextFileW over a host directory: the pattern's directory is
   listed and entries are matched against its last component ('*', '*.*', '*.ext', exact). */
typedef struct { DIR *dir; char base[1200], pattern[256]; } Find;
static int find_match(const char *pattern, const char *name) {
    if (!strcmp(pattern, "*") || !strcmp(pattern, "*.*")) return 1;
    if (pattern[0] == '*') {
        size_t lp = strlen(pattern + 1), ln = strlen(name);
        return ln >= lp && !_stricmp(name + ln - lp, pattern + 1);
    }
    return !_stricmp(pattern, name);
}
static int find_fill(Find *fd, uint32_t out) {
    struct dirent *e;
    while (fd->dir && (e = readdir(fd->dir))) {
        if (!find_match(fd->pattern, e->d_name)) continue;
        char full[1500];
        struct stat st;
        snprintf(full, sizeof full, "%s/%s", fd->base, e->d_name);
        int is_dir = !stat(full, &st) && (st.st_mode & S_IFDIR);
        memset(G(out), 0, 592);
        wr32(out, is_dir ? 0x10 : 0x80);
        if (!is_dir) wr32(out + 32, (uint32_t)st.st_size);
        u2w(e->d_name, out + 44, 260);
        return 1;
    }
    return 0;
}
static void find_first(Cpu *c, uint32_t pattern_w, uint32_t out, unsigned nargs) {
    char name[1024], path[1200];
    w2u(pattern_w, name, sizeof name);
    path_to_host(name, path, sizeof path);
    Find *fd = calloc(1, sizeof *fd);
    char *slash = strrchr(path, '/');
    snprintf(fd->pattern, sizeof fd->pattern, "%s", slash ? slash + 1 : path);
    if (slash) *slash = 0;
    snprintf(fd->base, sizeof fd->base, "%s", slash ? path : ".");
    fd->dir = opendir(fd->base);
    if (!find_fill(fd, out)) {
        if (fd->dir) closedir(fd->dir);
        free(fd);
        last_error = 2;
        ret_stdcall(c, 0xFFFFFFFFu, nargs);
        return;
    }
    ret_stdcall(c, handle_new(H_FIND, fd, 0), nargs);
}
API(FindFirstFileW) { find_first(c, ARG(c, 0), ARG(c, 1), 2); }
API(FindFirstFileExW) { find_first(c, ARG(c, 0), ARG(c, 2), 6); }
API(FindNextFileW) {
    Find *fd = handle_get(ARG(c, 0), H_FIND);
    int ok = fd && find_fill(fd, ARG(c, 1));
    if (!ok) last_error = 18; /* ERROR_NO_MORE_FILES */
    ret_stdcall(c, ok, 2);
}
API(FindClose) {
    Find *fd = handle_get(ARG(c, 0), H_FIND);
    if (fd) { if (fd->dir) closedir(fd->dir); free(fd); }
    handle_close(ARG(c, 0));
    ret_stdcall(c, 1, 1);
}

/* advapi32 SystemFunction036 (RtlGenRandom), found by rand_s through GetProcAddress for
   std::random_device: (buffer, length) -> BOOLEAN. */
API(SystemFunction036) {
    uint8_t *p = (uint8_t *)G(ARG(c, 0));
    for (uint32_t i = 0; i < ARG(c, 1); i++) p[i] = (uint8_t)(rand() >> 7);
    ret_stdcall(c, 1, 2);
}

#define KERNEL32_W_ENTRIES \
    E(SystemFunction036), \
    E(IsProcessorFeaturePresent), E(GetCurrentThread), E(GetThreadId), E(GetTickCount64), \
    E(QueryPerformanceFrequency), E(SleepEx), E(EncodePointer), E(DecodePointer), E(GetVersion), \
    E(VerSetConditionMask), E(VerifyVersionInfoA), {"VerifyVersionInfoW", VerifyVersionInfoA}, \
    E(GetNativeSystemInfo), E(GetStartupInfoW), E(GetCommandLineW), E(GetModuleFileNameW), \
    E(GetModuleHandleW), E(GetModuleHandleExW), E(LoadLibraryW), E(LoadLibraryExW), \
    E(FreeLibraryAndExitThread), E(ExitThread), E(GetEnvironmentVariableA), E(GetEnvironmentVariableW), \
    E(SetEnvironmentVariableW), E(ExpandEnvironmentStringsA), E(LocalFree), E(GetConsoleWindow), \
    E(PeekNamedPipe), E(ReadConsoleW), E(WriteConsoleW), E(ReadProcessMemory), E(DeviceIoControl), \
    E(GetThreadContext), E(SuspendThread), E(ResumeThread), E(SetThreadStackGuarantee), \
    E(SetThreadExecutionState), E(InitializeCriticalSectionEx), E(WaitForSingleObjectEx), \
    E(WaitForMultipleObjects), E(CreateEventW), E(ResetEvent), E(InitializeSRWLock), \
    E(AcquireSRWLockExclusive), E(ReleaseSRWLockExclusive), E(InitializeSListHead), \
    E(InterlockedPushEntrySList), E(GetLocaleInfoA), E(GetLocaleInfoW), E(GetLocaleInfoEx), \
    E(IsValidLocale), E(EnumSystemLocalesA), E(EnumSystemLocalesW), E(GetUserGeoID), E(GetGeoInfoA), \
    E(IsDBCSLeadByteEx), E(GetDateFormatW), E(GetTimeFormatW), E(GetTimeFormatA), E(GetCurrencyFormatW), \
    E(FormatMessageA), E(FormatMessageW), E(LCMapStringEx), E(FoldStringW), E(CompareStringW), \
    E(CompareStringEx), E(GetSystemTime), E(FileTimeToSystemTime), E(SystemTimeToTzSpecificLocalTime), \
    E(CreateFileW), E(GetFileAttributesW), E(CreateDirectoryW), E(RemoveDirectoryW), E(DeleteFileW), \
    E(MoveFileExW), E(CopyFileW), E(GetFullPathNameW), E(GetCurrentDirectoryW), E(GetWindowsDirectoryW), \
    E(GetSystemDirectoryA), E(GetDriveTypeW), E(AreFileApisANSI), E(GetFileSizeEx), E(SetFilePointerEx), \
    E(SetEndOfFile), E(GetFileInformationByHandle), E(FindFirstFileW), E(FindFirstFileExW), \
    E(FindNextFileW), E(FindClose),
