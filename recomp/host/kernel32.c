/* KERNEL32 for the recompiled game: the subset the MSVC 2005 CRT start-up and the
 * game's own initialisation need. Portable C (no host Win32 calls), so it can move
 * to the Vita unchanged. Threads are not started yet (CreateThread is recorded). */
#include "win32.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir_host(p) _mkdir(p)
#define file_seek fseeko64
#define file_tell ftello64
#else
#define mkdir_host(p) mkdir(p, 0755)
#define file_seek(f, o, w) fseek(f, (long)(o), w) /* game files are below 2 GiB */
#define file_tell(f) ((int64_t)ftell(f))
#endif

enum { TLS_SLOTS = 64, TIB_TLS = 0xE10 };
static uint8_t tls_used[TLS_SLOTS];
static uint32_t command_line, environment_a, environment_w;

#define API(name) static void name(Cpu *c)

/* -- process, time ------------------------------------------------------------------ */
API(GetCurrentProcessId) { ret_stdcall(c, 0x1000, 0); }
API(GetCurrentThreadId) { ret_stdcall(c, 0x1004, 0); }
API(GetCurrentProcess) { ret_stdcall(c, 0xFFFFFFFFu, 0); }
API(IsDebuggerPresent) { ret_stdcall(c, 0, 0); }
API(SetUnhandledExceptionFilter) { ret_stdcall(c, 0, 1); }
API(GetLastError) { ret_stdcall(c, last_error, 0); }
API(SetLastError) { last_error = ARG(c, 0); ret_stdcall(c, 0, 1); }
API(ExitProcess) { host_exit(c, ARG(c, 0), "ExitProcess"); }
API(TerminateProcess) { host_exit(c, ARG(c, 1), "TerminateProcess"); }
API(Sleep) { ret_stdcall(c, 0, 1); }

API(GetTickCount) { ret_stdcall(c, (uint32_t)(platform_ticks_us() / 1000u), 0); }
API(QueryPerformanceCounter) {
    wr64(ARG(c, 0), platform_ticks_us()); /* 1 MHz counter */
    ret_stdcall(c, 1, 1);
}
API(GetSystemTimeAsFileTime) {
    /* FILETIME: 100 ns since 1601. */
    uint64_t t = ((uint64_t)time(NULL) + 11644473600ull) * 10000000ull;
    wr64(ARG(c, 0), t);
    ret_stdcall(c, 0, 1);
}
API(GetTimeZoneInformation) {
    memset(G(ARG(c, 0)), 0, 172);
    wr32(ARG(c, 0), 0); /* Bias: UTC */
    ret_stdcall(c, 0 /* TIME_ZONE_ID_UNKNOWN */, 1);
}

/* -- heap and virtual memory ------------------------------------------------------- */
enum { HEAP_HANDLE = 0x00E00000u, HEAP_ZERO = 0x8 };
API(HeapCreate) { ret_stdcall(c, HEAP_HANDLE, 3); }
API(HeapDestroy) { ret_stdcall(c, 1, 1); }
API(GetProcessHeap) { ret_stdcall(c, HEAP_HANDLE, 0); }
API(HeapAlloc) { ret_stdcall(c, heap_alloc(ARG(c, 2), ARG(c, 1) & HEAP_ZERO), 3); }
static void check_heap_pointer(const char *api, uint32_t p) {
    if (p && heap_size(p) == 0xFFFFFFFFu) {
        uint32_t w[8];
        for (int k = 0; k < 8; k++) w[k] = rd32(p - 16 + 4 * k);
        trace("# %s(%08x): not a live heap block; header %08x %08x %08x %08x | %08x %08x %08x %08x", api, p,
              w[0], w[1], w[2], w[3], w[4], w[5], w[6], w[7]);
    }
}
API(HeapFree) { check_heap_pointer("HeapFree", ARG(c, 2)); heap_free(ARG(c, 2)); ret_stdcall(c, 1, 3); }
API(HeapReAlloc) { ret_stdcall(c, heap_realloc(ARG(c, 2), ARG(c, 3), ARG(c, 1) & HEAP_ZERO), 4); }
API(HeapSize) { check_heap_pointer("HeapSize", ARG(c, 2)); ret_stdcall(c, heap_size(ARG(c, 2)), 3); }
API(VirtualAlloc) { ret_stdcall(c, virtual_alloc(ARG(c, 0), ARG(c, 1)), 4); }
API(VirtualFree) { ret_stdcall(c, 1, 3); }

/* -- TLS: the slots live in the guest TIB, as Windows lays them out ---------------- */
API(TlsAlloc) {
    for (unsigned i = 0; i < TLS_SLOTS; i++) {
        if (!tls_used[i]) {
            tls_used[i] = 1;
            wr32(c->fs_base + TIB_TLS + 4 * i, 0);
            ret_stdcall(c, i, 0);
            return;
        }
    }
    ret_stdcall(c, 0xFFFFFFFFu, 0);
}
API(TlsFree) { if (ARG(c, 0) < TLS_SLOTS) tls_used[ARG(c, 0)] = 0; ret_stdcall(c, 1, 1); }
API(TlsGetValue) {
    uint32_t i = ARG(c, 0);
    last_error = 0;
    ret_stdcall(c, i < TLS_SLOTS ? rd32(c->fs_base + TIB_TLS + 4 * i) : 0, 1);
}
API(TlsSetValue) {
    uint32_t i = ARG(c, 0);
    if (i < TLS_SLOTS)
        wr32(c->fs_base + TIB_TLS + 4 * i, ARG(c, 1));
    ret_stdcall(c, i < TLS_SLOTS, 2);
}

/* -- modules -------------------------------------------------------------------------- */
API(GetModuleHandleA) {
    uint32_t name = ARG(c, 0);
    /* Only modules the game imports exist; others (mscoree.dll, probed by the CRT)
       are absent, as on a system without them. */
    const char *s = name ? STR(name) : NULL;
    uint32_t h = !s ? guest_image_base
               : !_stricmp(s, "kernel32.dll") || !_stricmp(s, "kernel32") ? FAKE_MODULE_KERNEL32
               : !_stricmp(s, "user32.dll") || !_stricmp(s, "user32") ? FAKE_MODULE_OTHER
               : 0;
    trace("GetModuleHandleA(%s) = %08x", name ? STR(name) : "NULL", h);
    ret_stdcall(c, h, 1);
}
API(LoadLibraryA) {
    trace("LoadLibraryA(%s)", STR(ARG(c, 0)));
    ret_stdcall(c, FAKE_MODULE_OTHER, 1);
}
API(FreeLibrary) { ret_stdcall(c, 1, 1); }
API(GetProcAddress) {
    uint32_t name = ARG(c, 1);
    uint32_t r = 0;
    if (name >= 0x10000) {
        r = api_address(STR(name));
        trace("GetProcAddress(%08x, %s) = %08x", ARG(c, 0), STR(name), r);
    }
    if (!r)
        last_error = 127; /* ERROR_PROC_NOT_FOUND */
    ret_stdcall(c, r, 2);
}
API(GetModuleFileNameA) {
    uint32_t buf = ARG(c, 1), n = ARG(c, 2);
    size_t len = strlen(guest_exe_path);
    if (n) {
        snprintf(STR(buf), n, "%s", guest_exe_path);
    }
    ret_stdcall(c, (uint32_t)(len < n ? len : n), 3);
}
API(GetCommandLineA) {
    if (!command_line)
        command_line = guest_strdup(guest_exe_path);
    ret_stdcall(c, command_line, 0);
}
API(GetStartupInfoA) {
    memset(G(ARG(c, 0)), 0, 68);
    wr32(ARG(c, 0), 68);
    ret_stdcall(c, 0, 1);
}
API(GetVersionExA) {
    uint32_t p = ARG(c, 0);
    wr32(p + 4, 5);    /* Windows XP SP3: 5.1.2600, VER_PLATFORM_WIN32_NT */
    wr32(p + 8, 1);
    wr32(p + 12, 2600);
    wr32(p + 16, 2);
    strcpy(STR(p + 20), "Service Pack 3");
    ret_stdcall(c, 1, 1);
}

/* -- environment ------------------------------------------------------------------------ */
API(GetEnvironmentStringsA) {
    if (!environment_a)
        environment_a = heap_alloc(2, 1);
    ret_stdcall(c, environment_a, 0);
}
API(GetEnvironmentStringsW) {
    if (!environment_w)
        environment_w = heap_alloc(4, 1);
    ret_stdcall(c, environment_w, 0);
}
API(FreeEnvironmentStrings) { ret_stdcall(c, 1, 1); }
API(SetEnvironmentVariableA) { ret_stdcall(c, 1, 2); }

/* -- synchronisation (single thread for now) --------------------------------------- */
API(CriticalSection1) { ret_stdcall(c, 0, 1); }
API(InitializeCriticalSectionAndSpinCount) { ret_stdcall(c, 1, 2); }
API(TryEnterCriticalSection) { ret_stdcall(c, 1, 1); }
API(InterlockedIncrement) {
    uint32_t p = ARG(c, 0);
    ret_stdcall(c, __atomic_add_fetch((uint32_t *)G(p), 1, __ATOMIC_SEQ_CST), 1);
}
API(InterlockedDecrement) {
    uint32_t p = ARG(c, 0);
    ret_stdcall(c, __atomic_sub_fetch((uint32_t *)G(p), 1, __ATOMIC_SEQ_CST), 1);
}
API(InterlockedExchange) {
    ret_stdcall(c, __atomic_exchange_n((uint32_t *)G(ARG(c, 0)), ARG(c, 1), __ATOMIC_SEQ_CST), 2);
}
API(InterlockedCompareExchange) {
    uint32_t expected = ARG(c, 2);
    __atomic_compare_exchange_n((uint32_t *)G(ARG(c, 0)), &expected, ARG(c, 1), 0,
                                __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    ret_stdcall(c, expected, 3);
}
API(CreateEventA) {
    trace("CreateEventA(manual=%u, initial=%u, %s)", ARG(c, 1), ARG(c, 2),
          ARG(c, 3) ? STR(ARG(c, 3)) : "NULL");
    ret_stdcall(c, handle_new(H_EVENT, NULL, ARG(c, 2)), 4);
}
API(CreateMutexA) {
    trace("CreateMutexA(%s)", ARG(c, 2) ? STR(ARG(c, 2)) : "NULL");
    last_error = 0;
    ret_stdcall(c, handle_new(H_MUTEX, NULL, 0), 3);
}
API(SetEvent) { ret_stdcall(c, 1, 1); }
API(WaitForSingleObject) {
    trace("WaitForSingleObject(%08x, %d)", ARG(c, 0), (int32_t)ARG(c, 1));
    ret_stdcall(c, 0 /* WAIT_OBJECT_0 */, 2);
}
API(CreateThread) {
    trace("CreateThread(start=%08x, param=%08x, flags=%08x): not started (single-threaded bring-up)",
          ARG(c, 2), ARG(c, 3), ARG(c, 4));
    if (ARG(c, 5))
        wr32(ARG(c, 5), 0x2000);
    ret_stdcall(c, handle_new(H_THREAD, NULL, ARG(c, 2)), 6);
}
API(SetThreadPriority) { ret_stdcall(c, 1, 2); }
API(GetExitCodeThread) { wr32(ARG(c, 1), 259 /* STILL_ACTIVE */); ret_stdcall(c, 1, 2); }
API(CloseHandle) { handle_close(ARG(c, 0)); ret_stdcall(c, 1, 1); }

/* -- code pages: an en-US Windows, ANSI code page 1252 (text.c) ----------------------- */
API(GetACP) { ret_stdcall(c, 1252, 0); }
API(GetOEMCP) { ret_stdcall(c, 437, 0); }
API(IsValidCodePage) { ret_stdcall(c, 1, 1); }
API(GetCPInfo) {
    uint32_t p = ARG(c, 1);
    memset(G(p), 0, 20);
    if (ARG(c, 0) == 65001) {
        wr32(p, 4);              /* MaxCharSize */
        wr8(p + 4, '?');         /* DefaultChar */
    } else {
        wr32(p, 1);
        wr8(p + 4, '?');
    }
    ret_stdcall(c, 1, 2);
}
API(GetUserDefaultLCID) { ret_stdcall(c, 0x0409 /* en-US */, 0); }

API(MultiByteToWideChar) {
    uint32_t src = ARG(c, 2), dst = ARG(c, 4), cap = ARG(c, 5);
    int32_t len = (int32_t)ARG(c, 3);
    int n = mb_to_utf16(ARG(c, 0), (const uint8_t *)G(src), len, cap ? (uint16_t *)G(dst) : NULL, (int)cap);
    if (!n && len) last_error = cap ? 122 /* ERROR_INSUFFICIENT_BUFFER */ : 87;
    ret_stdcall(c, (uint32_t)n, 6);
}
API(WideCharToMultiByte) {
    uint32_t src = ARG(c, 2), dst = ARG(c, 4), cap = ARG(c, 5), used = ARG(c, 7);
    int32_t len = (int32_t)ARG(c, 3);
    int defaulted = 0;
    int n = utf16_to_mb(ARG(c, 0), (const uint16_t *)G(src), len, cap ? (uint8_t *)G(dst) : NULL, (int)cap, &defaulted);
    if (used) wr32(used, (uint32_t)defaulted);
    if (!n && len) last_error = cap ? 122 : 87;
    ret_stdcall(c, (uint32_t)n, 8);
}
API(GetStringTypeW) {
    /* CT_CTYPE1 classification for ASCII; other characters are reported as alpha. */
    uint32_t src = ARG(c, 1), out = ARG(c, 3);
    int32_t len = (int32_t)ARG(c, 2);
    if (len < 0)
        for (len = 0; rd16(src + 2 * len); len++) {}
    for (int32_t i = 0; i < len; i++) {
        uint16_t ch = rd16(src + 2 * i), t = 0;
        if (ch < 0x80) {
            if (ch >= 'A' && ch <= 'Z') t |= 0x101;  /* UPPER | ALPHA */
            if (ch >= 'a' && ch <= 'z') t |= 0x102;  /* LOWER | ALPHA */
            if (ch >= '0' && ch <= '9') t |= 0x004 | 0x080; /* DIGIT | XDIGIT */
            if ((ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f')) t |= 0x080;
            if (ch == ' ' || (ch >= 9 && ch <= 13)) t |= 0x008; /* SPACE */
            if (ch == ' ' || ch == '\t') t |= 0x040; /* BLANK */
            if (ch < 32 || ch == 127) t |= 0x020;  /* CNTRL */
            if (ch > 32 && ch < 127 && !(t & 0x107)) t |= 0x010; /* PUNCT */
        } else {
            t = 0x100;
        }
        wr16(out + 2 * i, t);
    }
    ret_stdcall(c, 1, 4);
}
API(LCMapStringW) {
    /* Used by the CRT for case tables: identity copy of the source. */
    uint32_t src = ARG(c, 2), dst = ARG(c, 4), cap = ARG(c, 5);
    int32_t len = (int32_t)ARG(c, 3);
    if (len < 0) {
        for (len = 0; rd16(src + 2 * len); len++) {}
        len++;
    }
    uint32_t flags = ARG(c, 1);
    if (cap) {
        if ((uint32_t)len > cap) { ret_stdcall(c, 0, 6); return; }
        for (int32_t i = 0; i < len; i++) {
            uint16_t ch = rd16(src + 2 * i);
            if ((flags & 0x100) && ch >= 'A' && ch <= 'Z') ch += 32;   /* LCMAP_LOWERCASE */
            if ((flags & 0x200) && ch >= 'a' && ch <= 'z') ch -= 32;   /* LCMAP_UPPERCASE */
            wr16(dst + 2 * i, ch);
        }
    }
    ret_stdcall(c, (uint32_t)len, 6);
}

/* -- files ------------------------------------------------------------------------------- */
enum { STD_INPUT = 0xFFFFFFF6u, STD_OUTPUT = 0xFFFFFFF5u, STD_ERROR = 0xFFFFFFF4u };
API(GetStdHandle) { ret_stdcall(c, ARG(c, 0) == STD_INPUT ? 0 : handle_new(H_STD, NULL, ARG(c, 0)), 1); }
API(SetStdHandle) { ret_stdcall(c, 1, 2); }
API(GetFileType) { ret_stdcall(c, handle_get(ARG(c, 0), H_FILE) ? 1 : 2 /* CHAR */, 1); }
API(SetHandleCount) { ret_stdcall(c, ARG(c, 0), 1); }

/* Read-only files are kept open after CloseHandle and handed back on the next open of
   the same path: the game reopens th123a.dat ~740 times while loading, and each open
   costs ~10 ms on the Vita's memory card (measured). They also get a larger stdio
   buffer than newlib's default, which made every ReadFile a card access (44 s). */
enum { REUSE_SLOTS = 8, READ_BUFFER = 32 * 1024 };
static struct { char path[256]; FILE *f; int in_use; } reuse[REUSE_SLOTS];
static unsigned reuse_next;
static pthread_mutex_t reuse_lock = PTHREAD_MUTEX_INITIALIZER;

static FILE *open_read_only(const char *path) {
    FILE *f = NULL;
    pthread_mutex_lock(&reuse_lock);
    for (unsigned i = 0; i < REUSE_SLOTS && !f; i++)
        if (reuse[i].f && !reuse[i].in_use && !strcmp(reuse[i].path, path)) {
            reuse[i].in_use = 1;
            f = reuse[i].f;
        }
    pthread_mutex_unlock(&reuse_lock);
    if (f) {
        file_seek(f, 0, SEEK_SET);
        return f;
    }
    if (!(f = fopen(path, "rb")))
        return NULL;
    setvbuf(f, NULL, _IOFBF, READ_BUFFER);
    if (strlen(path) < sizeof reuse[0].path) {
        pthread_mutex_lock(&reuse_lock);
        for (unsigned k = 0; k < REUSE_SLOTS; k++) {
            unsigned i = (reuse_next + k) % REUSE_SLOTS;
            if (reuse[i].f && reuse[i].in_use)
                continue;
            if (reuse[i].f)
                fclose(reuse[i].f);   /* evict an idle one */
            snprintf(reuse[i].path, sizeof reuse[i].path, "%s", path);
            reuse[i].f = f;
            reuse[i].in_use = 1;
            reuse_next = i + 1;
            break;
        }
        pthread_mutex_unlock(&reuse_lock);
    }
    return f;
}

/* Called by handle_close for H_FILE: idle read-only files stay open for reuse. */
void file_close(FILE *f) {
    pthread_mutex_lock(&reuse_lock);
    for (unsigned i = 0; i < REUSE_SLOTS; i++)
        if (reuse[i].f == f) {
            reuse[i].in_use = 0;
            pthread_mutex_unlock(&reuse_lock);
            return;
        }
    pthread_mutex_unlock(&reuse_lock);
    fclose(f);
}

API(CreateFileA) {
    char path[1200];
    const char *name = STR(ARG(c, 0));
    path_to_host(name, path, sizeof path);
    uint32_t access = ARG(c, 1), disposition = ARG(c, 4);
    const char *mode = (access & 0x40000000u) ? (disposition == 3 || disposition == 4 ? "r+b" : "w+b")
                                               : "rb";
    FILE *f = (access & 0x40000000u) ? fopen(path, mode) : open_read_only(path);
    if (!f && (access & 0x40000000u) && disposition == 4)
        f = fopen(path, "w+b");
    uint32_t h = f ? handle_new(H_FILE, f, 0) : 0xFFFFFFFFu;
    last_error = f ? 0 : 2; /* ERROR_FILE_NOT_FOUND */
    trace("CreateFileA(%s, access=%08x, disposition=%u) = %08x (%s)", name, access, disposition, h, path);
    ret_stdcall(c, h, 7);
}
API(ReadFile) {
    FILE *f = handle_get(ARG(c, 0), H_FILE);
    uint32_t n = f ? (uint32_t)fread(G(ARG(c, 1)), 1, ARG(c, 2), f) : 0;
    if (ARG(c, 3))
        wr32(ARG(c, 3), n);
    ret_stdcall(c, f != NULL, 5);
}
API(WriteFile) {
    uint32_t h = ARG(c, 0), n = ARG(c, 2);
    FILE *f = handle_get(h, H_FILE);
    if (f)
        n = (uint32_t)fwrite(G(ARG(c, 1)), 1, n, f);
    else if (handle_get(h, H_STD) || !h)
        trace("WriteFile(std): %.*s", (int)n, STR(ARG(c, 1)));
    if (ARG(c, 3))
        wr32(ARG(c, 3), n);
    ret_stdcall(c, 1, 5);
}
API(SetFilePointer) {
    FILE *f = handle_get(ARG(c, 0), H_FILE);
    int64_t distance = (int32_t)ARG(c, 1);
    if (ARG(c, 2))
        distance = (int64_t)(((uint64_t)rd32(ARG(c, 2)) << 32) | ARG(c, 1));
    if (!f || file_seek(f, distance, (int)ARG(c, 3))) {
        ret_stdcall(c, 0xFFFFFFFFu, 4);
        return;
    }
    int64_t pos = file_tell(f);
    if (ARG(c, 2))
        wr32(ARG(c, 2), (uint32_t)((uint64_t)pos >> 32));
    ret_stdcall(c, (uint32_t)pos, 4);
}
API(GetFileSize) {
    FILE *f = handle_get(ARG(c, 0), H_FILE);
    if (!f) { ret_stdcall(c, 0xFFFFFFFFu, 2); return; }
    int64_t here = file_tell(f);
    file_seek(f, 0, SEEK_END);
    int64_t size = file_tell(f);
    file_seek(f, here, SEEK_SET);
    if (ARG(c, 1))
        wr32(ARG(c, 1), (uint32_t)((uint64_t)size >> 32));
    ret_stdcall(c, (uint32_t)size, 2);
}
API(FlushFileBuffers) {
    FILE *f = handle_get(ARG(c, 0), H_FILE);
    if (f) fflush(f);
    ret_stdcall(c, 1, 1);
}
API(GetFileAttributesA) {
    char path[1200];
    struct stat st;
    path_to_host(STR(ARG(c, 0)), path, sizeof path);
    uint32_t r = stat(path, &st) ? 0xFFFFFFFFu : (st.st_mode & S_IFDIR) ? 0x10 : 0x80;
    trace("GetFileAttributesA(%s) = %08x", STR(ARG(c, 0)), r);
    ret_stdcall(c, r, 1);
}
API(CreateDirectoryA) {
    char path[1200];
    path_to_host(STR(ARG(c, 0)), path, sizeof path);
    ret_stdcall(c, mkdir_host(path) == 0, 2);
}
API(DeleteFileA) {
    char path[1200];
    path_to_host(STR(ARG(c, 0)), path, sizeof path);
    ret_stdcall(c, remove(path) == 0, 1);
}
API(SetCurrentDirectoryA) {
    trace("SetCurrentDirectoryA(%s)", STR(ARG(c, 0)));
    ret_stdcall(c, 1, 1);
}
API(GetCurrentDirectoryA) {
    const char *dir = "C:\\Vagante";
    uint32_t n = ARG(c, 0);
    if (n > strlen(dir))
        strcpy(STR(ARG(c, 1)), dir);
    ret_stdcall(c, (uint32_t)strlen(dir), 2);
}
API(lstrlenA) { ret_stdcall(c, ARG(c, 0) ? (uint32_t)strlen(STR(ARG(c, 0))) : 0, 1); }

/* GetPrivateProfile*: the game reads its .ini; values come from the user's copy when
   present, otherwise the defaults the game passes. */
static int profile_lookup(const char *file, const char *section, const char *key, char *out, size_t n) {
    char path[1200], line[512], current[256] = "";
    path_to_host(file, path, sizeof path);
    FILE *f = fopen(path, "r");
    if (!f)
        return 0;
    int found = 0;
    while (!found && fgets(line, sizeof line, f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '[') {
            char *e = strchr(p, ']');
            if (e) { *e = 0; snprintf(current, sizeof current, "%s", p + 1); }
        } else if (!_stricmp(current, section)) {
            char *eq = strchr(p, '=');
            if (eq) {
                *eq = 0;
                char *k = p, *v = eq + 1;
                for (char *t = k + strlen(k); t > k && (t[-1] == ' ' || t[-1] == '\t'); ) *--t = 0;
                v[strcspn(v, "\r\n")] = 0;
                if (!_stricmp(k, key)) { snprintf(out, n, "%s", v); found = 1; }
            }
        }
    }
    fclose(f);
    return found;
}
API(GetPrivateProfileIntA) {
    char value[256];
    uint32_t r = ARG(c, 2);
    if (profile_lookup(STR(ARG(c, 3)), STR(ARG(c, 0)), STR(ARG(c, 1)), value, sizeof value))
        r = (uint32_t)strtol(value, NULL, 10);
    trace("GetPrivateProfileIntA([%s] %s, %s) = %d", STR(ARG(c, 0)), STR(ARG(c, 1)), STR(ARG(c, 3)), (int)r);
    ret_stdcall(c, r, 4);
}
API(GetPrivateProfileStringA) {
    char value[256];
    uint32_t buf = ARG(c, 3), n = ARG(c, 4);
    const char *def = ARG(c, 2) ? STR(ARG(c, 2)) : "";
    if (!ARG(c, 0) || !ARG(c, 1) ||
        !profile_lookup(STR(ARG(c, 5)), STR(ARG(c, 0)), STR(ARG(c, 1)), value, sizeof value))
        snprintf(value, sizeof value, "%s", def);
    if (n)
        snprintf(STR(buf), n, "%s", value);
    trace("GetPrivateProfileStringA([%s] %s) = \"%s\"", ARG(c, 0) ? STR(ARG(c, 0)) : "",
          ARG(c, 1) ? STR(ARG(c, 1)) : "", value);
    ret_stdcall(c, n ? (uint32_t)strlen(STR(buf)) : 0, 6);
}

/* -- console (CRT error output only) ------------------------------------------------- */
API(GetConsoleCP) { ret_stdcall(c, 437, 0); }
API(GetConsoleOutputCP) { ret_stdcall(c, 437, 0); }
API(GetConsoleMode) { ret_stdcall(c, 0, 2); }

/* -- exceptions: dispatch and unwind are in seh.c --------------------------------------- */
void seh_raise(Cpu *c);
void seh_rtl_unwind(Cpu *c);
/* The CRT reaches this on a fatal error (stack cookie, invalid parameter, abort):
   report the thread and its last guest functions, then stop. */
API(UnhandledExceptionFilter) {
    uint32_t record = ARG(c, 0) ? rd32(ARG(c, 0)) : 0;
    trace("# UnhandledExceptionFilter on thread %u: exception %08x at %08x", rd32(c->fs_base + 0x24),
          record ? rd32(record) : 0, record ? rd32(record + 12) : 0);
    trace("# last guest functions entered on this thread (oldest first):");
    for (unsigned k = 0; k < 64; k++) {
        uint32_t f = guest_trace_ring[(guest_trace_pos + k) & 63];
        if (f) trace("#   %08x", f);
    }
    host_exit(c, 0xC0000409u, "UnhandledExceptionFilter: fatal CRT error in the guest");
}
API(RaiseException) {
    if (ARG(c, 0) == 0x406D1388u) { /* MS_VC_EXCEPTION: thread naming for a debugger; continue */
        ret_stdcall(c, 0, 4);
        return;
    }
    seh_raise(c);
}
API(RtlUnwind) { seh_rtl_unwind(c); }

/* Wide-character, locale and newer APIs vagante.exe imports (they share the helpers above). */
#include "kernel32_w.inc"

#define E(name) {#name, name}
const ApiEntry win32_kernel32[] = {
    E(GetCurrentProcessId), E(GetCurrentThreadId), E(GetCurrentProcess), E(IsDebuggerPresent),
    E(SetUnhandledExceptionFilter), E(GetLastError), E(SetLastError), E(ExitProcess),
    E(TerminateProcess), E(Sleep), E(GetTickCount), E(QueryPerformanceCounter),
    E(GetSystemTimeAsFileTime), E(GetTimeZoneInformation),
    E(HeapCreate), E(HeapDestroy), E(GetProcessHeap), E(HeapAlloc), E(HeapFree), E(HeapReAlloc),
    E(HeapSize), E(VirtualAlloc), E(VirtualFree),
    E(TlsAlloc), E(TlsFree), E(TlsGetValue), E(TlsSetValue),
    E(GetModuleHandleA), E(LoadLibraryA), E(FreeLibrary), E(GetProcAddress), E(GetModuleFileNameA),
    E(GetCommandLineA), E(GetStartupInfoA), E(GetVersionExA),
    {"GetEnvironmentStrings", GetEnvironmentStringsA}, E(GetEnvironmentStringsW),
    {"FreeEnvironmentStringsA", FreeEnvironmentStrings}, {"FreeEnvironmentStringsW", FreeEnvironmentStrings},
    E(SetEnvironmentVariableA),
    {"InitializeCriticalSection", CriticalSection1}, {"DeleteCriticalSection", CriticalSection1},
    {"EnterCriticalSection", CriticalSection1}, {"LeaveCriticalSection", CriticalSection1},
    E(InitializeCriticalSectionAndSpinCount),
    E(TryEnterCriticalSection), E(InterlockedIncrement), E(InterlockedDecrement), E(InterlockedExchange),
    E(InterlockedCompareExchange), E(CreateEventA), E(CreateMutexA), E(SetEvent), E(WaitForSingleObject),
    E(CreateThread), E(SetThreadPriority), E(GetExitCodeThread), E(CloseHandle),
    E(GetACP), E(GetOEMCP), E(IsValidCodePage), E(GetCPInfo), E(GetUserDefaultLCID),
    E(MultiByteToWideChar), E(WideCharToMultiByte), E(GetStringTypeW), E(LCMapStringW),
    E(GetStdHandle), E(SetStdHandle), E(GetFileType), E(SetHandleCount), E(CreateFileA), E(ReadFile),
    E(WriteFile), E(SetFilePointer), E(GetFileSize), E(FlushFileBuffers), E(GetFileAttributesA),
    E(CreateDirectoryA), E(DeleteFileA), E(SetCurrentDirectoryA), E(GetCurrentDirectoryA), E(lstrlenA),
    E(GetPrivateProfileIntA), E(GetPrivateProfileStringA),
    E(GetConsoleCP), E(GetConsoleOutputCP), E(GetConsoleMode), E(RaiseException), E(RtlUnwind), E(UnhandledExceptionFilter),
    KERNEL32_W_ENTRIES
    {NULL, NULL},
};
