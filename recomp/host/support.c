/* Host support: guest heap, virtual memory, handles, path mapping, imports, traces. */
#include "win32.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

/* -- traces ---------------------------------------------------------------------------- */
static FILE *trace_file;
unsigned long long import_calls;
char trace_path[256] = "host-trace.log";

void trace(const char *fmt, ...) {
    char line[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    static pthread_mutex_t trace_lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&trace_lock);
    if (!trace_file) {
        trace_file = fopen(trace_path, "w");
        if (trace_file)
            setvbuf(trace_file, NULL, _IOFBF, 64 * 1024);
    }
    if (trace_file) {
        fputs(line, trace_file);
        fputc('\n', trace_file);
        /* Flushing every line cost ~10 ms per line on the Vita's card (8.7 s of
           CreateFileA alone). Milestones and errors ('#') still go out at once, so a
           crash report keeps them; plain call lines go out at least once a second. */
        static uint64_t last_flush;
        uint64_t now = platform_ticks_us();
        if (line[0] == '#' || now - last_flush > 1000000u) {
            fflush(trace_file);
            last_flush = now;
        }
    }
    platform_log(line);
    pthread_mutex_unlock(&trace_lock);
}

void trace_close(void) {
    if (trace_file) {
        fclose(trace_file);
        trace_file = NULL;
    }
}

/* -- guest heap -------------------------------------------------------------------------
 * Size-class free lists (16-byte classes up to 4 KiB) plus a first-fit list for large
 * blocks; a 16-byte header before every block holds the requested size. Large blocks are
 * split on allocation and coalesced with their free neighbours (the DAT loader churns
 * multi-MiB buffers; without it 96 MiB were exhausted during boot). */
enum { HDR = 16, CLASS_STEP = 16, CLASSES = 256, MAGIC = 0x48454150u /* "HEAP" */ };
static uint32_t heap_top = GUEST_HEAP_BASE, heap_committed = GUEST_HEAP_BASE;
static uint32_t free_class[CLASSES + 1];
static uint32_t free_large;
static uint32_t heap_live;

static uint32_t round_up(uint32_t n) { return (n + CLASS_STEP - 1) & ~(uint32_t)(CLASS_STEP - 1); }

static uint32_t block_capacity(uint32_t p) { return rd32(p - HDR + 8); }

static uint32_t carve(uint32_t capacity) {
    uint32_t block = heap_top;
    if (capacity > GUEST_HEAP_BASE + GUEST_HEAP_SIZE - block - HDR) /* remaining space, no wrap */
        return 0;
    uint32_t end = block + HDR + capacity;
    while (heap_committed < end) {
        if (guest_map(heap_committed, 0x100000))
            return 0;
        heap_committed += 0x100000;
    }
    heap_top = end;
    wr32(block + 8, capacity);
    return block + HDR;
}

static uint32_t heap_alloc_locked(uint32_t size, int zero) {
    /* larger than the whole heap: refuse before round_up and carve can wrap around
       (Codex T10 found both overflows) */
    if (size > GUEST_HEAP_SIZE - HDR)
        return 0;
    uint32_t capacity = round_up(size ? size : 1), p = 0;
    if (capacity / CLASS_STEP <= CLASSES) {
        uint32_t k = capacity / CLASS_STEP;
        if (free_class[k]) {
            p = free_class[k];
            free_class[k] = rd32(p);
        }
    } else {
        /* first fit in the address-ordered list; the tail is split off when it can
           still hold a large block */
        uint32_t prev = 0;
        for (uint32_t q = free_large; q; prev = q, q = rd32(q)) {
            uint32_t have = block_capacity(q);
            if (have < capacity)
                continue;
            uint32_t next = rd32(q);
            if (have - capacity >= HDR + (CLASSES + 1) * CLASS_STEP) {
                uint32_t rest = q + capacity + HDR;
                wr32(rest - HDR + 8, have - capacity - HDR);
                wr32(rest - HDR + 4, 0);
                wr32(rest, next);
                next = rest;
                wr32(q - HDR + 8, capacity);
            }
            if (prev) wr32(prev, next); else free_large = next;
            p = q;
            break;
        }
    }
    if (!p)
        p = carve(capacity);
    if (!p) {
        trace("# heap exhausted: request %u bytes, live %u KiB", size, heap_live >> 10);
        return 0;
    }
    heap_live += block_capacity(p);
    wr32(p - HDR, size);
    wr32(p - HDR + 4, MAGIC);
    if (zero)
        memset(G(p), 0, block_capacity(p));
    return p;
}

static void heap_free_locked(uint32_t p) {
    if (!p || rd32(p - HDR + 4) != MAGIC)
        return;
    wr32(p - HDR + 4, 0);
    uint32_t capacity = block_capacity(p);
    heap_live -= capacity;
    if (capacity / CLASS_STEP <= CLASSES) {
        wr32(p, free_class[capacity / CLASS_STEP]);
        free_class[capacity / CLASS_STEP] = p;
    } else {
        /* address-ordered insert, merging with the neighbours that touch */
        uint32_t prev = 0, q = free_large;
        while (q && q < p) { prev = q; q = rd32(q); }
        if (q && p + capacity + HDR == q) {
            capacity += HDR + block_capacity(q);
            q = rd32(q);
        }
        if (prev && prev + block_capacity(prev) + HDR == p) {
            wr32(prev - HDR + 8, block_capacity(prev) + HDR + capacity);
            wr32(prev, q);
            return;
        }
        wr32(p - HDR + 8, capacity);
        wr32(p, q);
        if (prev) wr32(prev, p); else free_large = p;
    }
}

uint32_t heap_size(uint32_t p) {
    return p && rd32(p - HDR + 4) == MAGIC ? rd32(p - HDR) : 0xFFFFFFFFu;
}

static uint32_t heap_realloc_locked(uint32_t p, uint32_t size, int zero) {
    if (!p)
        return heap_alloc_locked(size, zero);
    uint32_t old = heap_size(p);
    if (size <= block_capacity(p)) {
        if (zero && size > old)
            memset(G(p + old), 0, size - old);
        wr32(p - HDR, size);
        return p;
    }
    uint32_t q = heap_alloc_locked(size, zero);
    if (!q)
        return 0;
    memcpy(G(q), G(p), old);
    heap_free_locked(p);
    return q;
}

/* The guest heap is shared by all guest threads. */
static pthread_mutex_t heap_lock = PTHREAD_MUTEX_INITIALIZER;
uint32_t heap_alloc(uint32_t size, int zero) {
    pthread_mutex_lock(&heap_lock);
    uint32_t p = heap_alloc_locked(size, zero);
    pthread_mutex_unlock(&heap_lock);
    return p;
}
void heap_free(uint32_t p) {
    pthread_mutex_lock(&heap_lock);
    heap_free_locked(p);
    pthread_mutex_unlock(&heap_lock);
}
uint32_t heap_realloc(uint32_t p, uint32_t size, int zero) {
    pthread_mutex_lock(&heap_lock);
    uint32_t q = heap_realloc_locked(p, size, zero);
    pthread_mutex_unlock(&heap_lock);
    return q;
}

uint32_t heap_used(void) { return heap_top - GUEST_HEAP_BASE; }

uint32_t guest_strdup(const char *s) {
    uint32_t p = heap_alloc((uint32_t)strlen(s) + 1, 0);
    if (p)
        strcpy(STR(p), s);
    return p;
}

/* -- virtual memory: bump allocation in its own region, committed on request -------- */
static uint32_t virtual_top = GUEST_VIRTUAL_BASE;

uint32_t virtual_used(void) { return virtual_top - GUEST_VIRTUAL_BASE; }

uint32_t virtual_alloc(uint32_t address, uint32_t size) {
    size = (size + 0xFFFF) & ~0xFFFFu;
    if (address) {
        /* Commit inside an earlier reservation. */
        return guest_map(address, size) ? 0 : address;
    }
    if (virtual_top + size > GUEST_VIRTUAL_BASE + GUEST_VIRTUAL_SIZE || guest_map(virtual_top, size))
        return 0;
    uint32_t r = virtual_top;
    virtual_top += size;
    return r;
}

/* -- handles --------------------------------------------------------------------------- */
typedef struct { int type; void *host; uint32_t extra; } Handle;
static Handle handles[4096];
uint32_t last_error;

static pthread_mutex_t handle_lock = PTHREAD_MUTEX_INITIALIZER;

uint32_t handle_new(int type, void *host, uint32_t extra) {
    pthread_mutex_lock(&handle_lock);
    for (unsigned i = 1; i < 4096; i++) {
        if (!handles[i].type) {
            handles[i] = (Handle){type, host, extra};
            pthread_mutex_unlock(&handle_lock);
            return 0x1000u + i * 4;
        }
    }
    pthread_mutex_unlock(&handle_lock);
    return 0;
}

static Handle *slot(uint32_t h) {
    if (h < 0x1000u || ((h - 0x1000u) & 3))
        return NULL;
    unsigned i = (h - 0x1000u) / 4;
    return i < 4096 && handles[i].type ? &handles[i] : NULL;
}

void *handle_get(uint32_t h, int type) {
    Handle *s = slot(h);
    return s && s->type == type ? s->host : NULL;
}

void handle_close(uint32_t h) {
    Handle *s = slot(h);
    if (!s)
        return;
    if (s->type == H_FILE && s->host)
        file_close(s->host);
    memset(s, 0, sizeof *s);
}

/* -- paths ------------------------------------------------------------------------------- */
char host_data_dir[1024] = ".";
const char *guest_exe_path = "C:\\Vagante\\vagante.exe";

void path_to_host(const char *guest_path, char *out, size_t size) {
    const char *p = guest_path;
    static const char prefix[] = "C:\\Vagante\\";
    if (!_strnicmp(p, prefix, sizeof prefix - 1))
        p += sizeof prefix - 1;
    else if (p[0] && p[1] == ':')
        p += 2; /* other absolute paths are kept under the data directory */
    while (*p == '\\' || *p == '/')
        p++;
    snprintf(out, size, "%s/%s", host_data_dir, p);
    for (char *q = out; *q; q++)
        if (*q == '\\')
            *q = '/';
}

/* -- imports ------------------------------------------------------------------------------ */
typedef struct { char name[64]; GuestFn fn; unsigned long long calls, us; } Fake; /* us: host time inside */
static Fake fakes[1024];
static unsigned fake_count;
static unsigned recent[64], recent_pos;

/* Every fake address dispatches here: count, remember, then run the implementation. */
static void api_dispatch(Cpu *c) {
    unsigned i = (guest_current_target - GUEST_FAKE_BASE) / 16;
    fakes[i].calls++;
    import_calls++;
    recent[recent_pos++ % 64] = i;
    uint64_t t0 = platform_ticks_us();
    fakes[i].fn(c);
    fakes[i].us += platform_ticks_us() - t0;
}

void trace_summary(void) {
    trace("# memory: heap %u KiB, VirtualAlloc %u KiB", heap_used() / 1024, virtual_used() / 1024);
    trace("# Win32 calls by function:");
    for (unsigned i = 0; i < fake_count; i++)
        if (fakes[i].calls)
            trace("#   %8llu %s (%llu ms)", fakes[i].calls, fakes[i].name, fakes[i].us / 1000);
    trace("# last calls (oldest first):");
    unsigned n = recent_pos < 64 ? recent_pos : 64;
    for (unsigned k = 0; k < n; k++)
        trace("#   %s", fakes[recent[(recent_pos - n + k) % 64]].name);
}

static void unimplemented(Cpu *c) {
    const char *name = api_name(guest_current_target);
    static char why[160]; /* host_exit keeps the pointer past this frame */
    snprintf(why, sizeof why, "import %s is not implemented", name ? name : "?");
    host_exit(c, 0xDEAD, why);
}

/* Exports with fixed, documented ordinals that vagante.exe imports by number. */
static const char *ordinal_name(const char *dll, unsigned ordinal) {
    static const char *ws2_32[] = {
        [1] = "accept", [2] = "bind", [3] = "closesocket", [4] = "connect", [5] = "getpeername",
        [6] = "getsockname", [7] = "getsockopt", [8] = "htonl", [9] = "htons", [10] = "ioctlsocket",
        [11] = "inet_addr", [12] = "inet_ntoa", [13] = "listen", [14] = "ntohl", [15] = "ntohs",
        [16] = "recv", [17] = "recvfrom", [18] = "select", [19] = "send", [20] = "sendto",
        [21] = "setsockopt", [23] = "socket", [57] = "gethostname", [111] = "WSAGetLastError",
        [112] = "WSASetLastError", [115] = "WSAStartup", [116] = "WSACleanup", [151] = "__WSAFDIsSet",
    };
    if (!_stricmp(dll, "WS2_32.dll") && ordinal < sizeof ws2_32 / sizeof ws2_32[0])
        return ws2_32[ordinal];
    if (!_stricmp(dll, "OLEAUT32.dll"))
        return ordinal == 2 ? "SysAllocString" : ordinal == 6 ? "SysFreeString" : NULL;
    return NULL;
}

static GuestFn lookup(const char *name) {
    const ApiEntry *tables[] = {win32_threads, win32_kernel32, win32_user, win32_stubs, win32_openal, win32_gl, win32_window};
    for (unsigned t = 0; t < sizeof tables / sizeof tables[0]; t++)
        for (const ApiEntry *e = tables[t]; e->name; e++)
            if (!strcmp(e->name, name))
                return e->fn;
    return NULL;
}

static uint32_t fake_for(const char *name, int create_unimplemented) {
    for (unsigned i = 0; i < fake_count; i++)
        if (!strcmp(fakes[i].name, name))
            return GUEST_FAKE_BASE + 16u * i;
    GuestFn fn = lookup(name);
    if (!fn && !create_unimplemented)
        return 0;
    if (fake_count == 1024)
        return 0;
    snprintf(fakes[fake_count].name, sizeof fakes[0].name, "%s", name);
    fakes[fake_count].fn = fn ? fn : unimplemented;
    uint32_t address = GUEST_FAKE_BASE + 16u * fake_count;
    guest_intercept(address, api_dispatch);
    fake_count++;
    return address;
}

const char *api_name(uint32_t fake) {
    if (fake < GUEST_FAKE_BASE || (fake - GUEST_FAKE_BASE) / 16 >= fake_count)
        return NULL;
    return fakes[(fake - GUEST_FAKE_BASE) / 16].name;
}

uint32_t api_address(const char *name) { return fake_for(name, 0); }

int imports_init(void) {
    uint32_t base = guest_image_base;
    uint32_t pe = base + rd32(base + 0x3C);
    uint32_t dir = rd32(pe + 24 + 104); /* optional header: import directory RVA */
    unsigned total = 0, implemented = 0;
    for (uint32_t d = base + dir; rd32(d + 12); d += 20) {
        uint32_t names = rd32(d) ? rd32(d) : rd32(d + 16);
        uint32_t iat = base + rd32(d + 16);
        for (unsigned i = 0;; i++) {
            uint32_t thunk = rd32(base + names + 4 * i);
            if (!thunk)
                break;
            char name[64];
            const char *by_ordinal = (thunk & 0x80000000u) ? ordinal_name(STR(base + rd32(d + 12)), thunk & 0xFFFF) : NULL;
            if (by_ordinal)
                snprintf(name, sizeof name, "%s", by_ordinal);
            else if (thunk & 0x80000000u)
                snprintf(name, sizeof name, "%s#%u", STR(base + rd32(d + 12)), thunk & 0xFFFF);
            else
                snprintf(name, sizeof name, "%s", STR(base + thunk + 2));
            wr32(iat + 4 * i, fake_for(name, 1));
            total++;
            implemented += lookup(name) != NULL;
        }
    }
    trace("# imports: %u, implemented: %u", total, implemented);
    return 0;
}

/* -- implicit TLS -------------------------------------------------------------------------- */
void tls_init_thread(uint32_t tib) {
    uint32_t pe = guest_image_base + rd32(guest_image_base + 0x3C);
    uint32_t dir_rva = rd32(pe + 24 + 96 + 9 * 8); /* IMAGE_DIRECTORY_ENTRY_TLS */
    wr32(tib + 0x2C, tib + 0xF10);
    if (!dir_rva)
        return;
    uint32_t dir = guest_image_base + dir_rva;
    uint32_t start = rd32(dir), end = rd32(dir + 4), index_at = rd32(dir + 8), zero_fill = rd32(dir + 16);
    uint32_t block = heap_alloc(end - start + zero_fill + 16, 1);
    memcpy(G(block), G(start), end - start);
    wr32(index_at, 0);
    wr32(tib + 0xF10, block);
}