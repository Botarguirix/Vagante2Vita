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

/* -- guest heap: 32-bit-offset TLSF -------------------------------------------------------
 * Two-level segregated free lists: 32 SL bins per power of two, bitmap selection,
 * immediate physical-neighbour coalescing. No free-list traversal on alloc/free.
 * Header (16 bytes): requested size, allocation magic, capacity, previous header.
 * Free blocks store two uint32 guest links in their unused payload. There are no
 * host pointers in the arena, so PC x64 and Vita ARM use the identical layout.
 * Only metadata/mapping is locked; zeroing and moving copies run after unlocking.
 * TLSF bounds metadata work, not page commitment, memset, memcpy or diagnostics.
 */
enum { HDR=16, ALIGN=16, SL_BITS=5, SL_COUNT=32, FL_COUNT=24,
       SMALL_LIMIT=ALIGN*SL_COUNT, MAGIC=0x48454150u, TOP_ALLOCATIONS=8 };
_Static_assert((GUEST_HEAP_BASE & 15)==0 && (GUEST_HEAP_SIZE & 15)==0, "TLSF arena alignment");
#define HEAP_END ((uint32_t)(GUEST_HEAP_BASE+GUEST_HEAP_SIZE))
static uint32_t heap_committed=GUEST_HEAP_BASE;
static uint32_t heap_fl_bitmap, heap_sl_bitmap[FL_COUNT], heap_heads[FL_COUNT][SL_COUNT];
static uint32_t heap_live, heap_peak, heap_max_request;
static unsigned heap_initialized, heap_map_failures;
static uint64_t heap_alloc_calls, heap_free_calls, heap_realloc_calls;
static pthread_mutex_t heap_lock=PTHREAD_MUTEX_INITIALIZER;

static uint32_t block_capacity(uint32_t b) { return rd32(b+8); }
static unsigned high_bit(uint32_t n) { return 31u-(unsigned)__builtin_clz(n); }
static void heap_bin(uint32_t n,unsigned *fl,unsigned *sl) {
    if(n<SMALL_LIMIT) { *fl=0; *sl=n/ALIGN; }
    else {
        unsigned bit=high_bit(n);
        *fl=bit-8; *sl=(n>>(bit-SL_BITS))-SL_COUNT;
    }
}
static void heap_insert(uint32_t b) {
    unsigned fl,sl; heap_bin(block_capacity(b),&fl,&sl);
    uint32_t head=heap_heads[fl][sl];
    wr32(b+HDR,head); wr32(b+HDR+4,0);
    if(head) wr32(head+HDR+4,b);
    heap_heads[fl][sl]=b;
    heap_sl_bitmap[fl]|=1u<<sl; heap_fl_bitmap|=1u<<fl;
}
static void heap_unlink(uint32_t b) {
    unsigned fl,sl; heap_bin(block_capacity(b),&fl,&sl);
    uint32_t next=rd32(b+HDR),previous=rd32(b+HDR+4);
    if(previous) wr32(previous+HDR,next); else heap_heads[fl][sl]=next;
    if(next) wr32(next+HDR+4,previous);
    if(!heap_heads[fl][sl]) {
        heap_sl_bitmap[fl]&=~(1u<<sl);
        if(!heap_sl_bitmap[fl]) heap_fl_bitmap&=~(1u<<fl);
    }
}
static uint32_t heap_find(uint32_t need) {
    unsigned fl,sl; heap_bin(need,&fl,&sl);
    /* A single head probe preserves exact fits at the top of an arena/bin.
       Never scan the remainder of this bin. Otherwise round the search upward
       so every block in the selected bin is guaranteed to fit. */
    uint32_t b=heap_heads[fl][sl];
    if(b && block_capacity(b)>=need) return b;
    if(need>=SMALL_LIMIT) {
        uint32_t round=(1u<<(high_bit(need)-SL_BITS))-1;
        if(need>UINT32_MAX-round) return 0;
        heap_bin(need+round,&fl,&sl);
    }
    if(fl>=FL_COUNT) return 0;
    uint32_t slots=heap_sl_bitmap[fl] & (UINT32_MAX<<sl);
    if(!slots) {
        uint32_t levels=heap_fl_bitmap & (UINT32_MAX<<(fl+1));
        if(!levels) return 0;
        fl=(unsigned)__builtin_ctz(levels); slots=heap_sl_bitmap[fl];
    }
    sl=(unsigned)__builtin_ctz(slots);
    return heap_heads[fl][sl];
}
static int heap_commit(uint32_t end) {
    if(end<=heap_committed) return 1;
    if(end>HEAP_END) return 0;
    uint32_t limit=(end+0xfffffu)&~0xfffffu;
    if(limit>HEAP_END) limit=HEAP_END;
    if(guest_map(heap_committed,limit-heap_committed)) { ++heap_map_failures; return 0; }
    heap_committed=limit; return 1;
}
static int heap_init(void) {
    if(heap_initialized) return 1;
    if(!heap_commit(GUEST_HEAP_BASE+HDR+ALIGN)) return 0;
    uint32_t b=GUEST_HEAP_BASE;
    wr32(b,0); wr32(b+4,0); wr32(b+8,GUEST_HEAP_SIZE-HDR); wr32(b+12,0);
    heap_insert(b); heap_initialized=1; return 1;
}
static int heap_valid(uint32_t p) {
    if(!heap_initialized || (p&15) || p<GUEST_HEAP_BASE+HDR || p>=heap_committed) return 0;
    uint32_t b=p-HDR;
    if(rd32(b+4)!=MAGIC) return 0;
    uint32_t cap=block_capacity(b);
    return cap>=ALIGN && !(cap&15) && cap<=HEAP_END-p && rd32(b)<=cap;
}
static void heap_fix_next(uint32_t b) {
    uint32_t next=b+HDR+block_capacity(b);
    if(next<HEAP_END) wr32(next+12,b);
}
/* b is not in a free list yet; its free successor, if any, is in one. */
static void heap_join_next(uint32_t b) {
    uint32_t next=b+HDR+block_capacity(b);
    if(next<HEAP_END && rd32(next+4)==0) {
        heap_unlink(next); wr32(b+8,block_capacity(b)+HDR+block_capacity(next));
        heap_fix_next(b);
    }
}
static void heap_split(uint32_t b,uint32_t need) {
    uint32_t cap=block_capacity(b);
    if(cap-need<HDR+ALIGN) return;
    uint32_t tail=b+HDR+need;
    wr32(b+8,need);
    wr32(tail,0); wr32(tail+4,0); wr32(tail+8,cap-need-HDR); wr32(tail+12,b);
    heap_fix_next(tail); heap_join_next(tail); heap_insert(tail);
}
static int heap_prepare(uint32_t b,uint32_t cap,uint32_t need) {
    /* Commit all returned payload plus a split tail's header and free links. */
    int split=cap-need>=HDR+ALIGN;
    return heap_commit(b+HDR+(split?need:cap)+(split?HDR+ALIGN:0));
}
static uint32_t heap_alloc_locked(uint32_t size) {
    if(size>GUEST_HEAP_SIZE-HDR) return 0;
    uint32_t need=((size?size:1)+15u)&~15u;
    if(!heap_init()) return 0;
    uint32_t b=heap_find(need);
    if(!b || !heap_prepare(b,block_capacity(b),need)) return 0;
    heap_unlink(b); heap_split(b,need);
    wr32(b,size); wr32(b+4,MAGIC);
    heap_live+=block_capacity(b);
    if(heap_live>heap_peak) heap_peak=heap_live;
    return b+HDR;
}
static void heap_free_locked(uint32_t p) {
    if(!heap_valid(p)) return;
    uint32_t b=p-HDR;
    heap_live-=block_capacity(b); wr32(b,0); wr32(b+4,0);
    uint32_t previous=rd32(b+12);
    if(previous && rd32(previous+4)==0) {
        heap_unlink(previous);
        wr32(previous+8,block_capacity(previous)+HDR+block_capacity(b));
        b=previous; heap_fix_next(b);
    }
    heap_join_next(b); heap_insert(b);
}

/* Cold diagnostics only: exact largest free block and top live allocations.
   Walk once under the lock, then release it before tracing/formatting. */
static void heap_trace_stats(void) {
    struct Allocation { uint32_t address,size,capacity; } top[TOP_ALLOCATIONS]={{0}};
    uint32_t largest=GUEST_HEAP_SIZE-HDR,live,peak,committed,request;
    unsigned maps;
    uint64_t allocations,frees,reallocations;
    pthread_mutex_lock(&heap_lock);
    live=heap_live; peak=heap_peak; committed=heap_committed-GUEST_HEAP_BASE;
    request=heap_max_request; maps=heap_map_failures;
    allocations=heap_alloc_calls; frees=heap_free_calls; reallocations=heap_realloc_calls;
    if(heap_initialized) {
        largest=0;
        for(uint32_t b=GUEST_HEAP_BASE;b<HEAP_END;) {
            uint32_t cap=block_capacity(b);
            if(cap<ALIGN || (cap&15) || cap>HEAP_END-b-HDR) break;
            if(rd32(b+4)==0) { if(cap>largest) largest=cap; }
            else if(rd32(b+4)==MAGIC) {
                uint32_t n=rd32(b);
                for(unsigned i=0;i<TOP_ALLOCATIONS;i++) if(!top[i].address || n>top[i].size) {
                    for(unsigned j=TOP_ALLOCATIONS-1;j>i;j--) top[j]=top[j-1];
                    top[i]=(struct Allocation){b+HDR,n,cap}; break;
                }
            }
            b+=HDR+cap;
        }
    }
    pthread_mutex_unlock(&heap_lock);
    trace("# heap TLSF: live %u KiB, peak %u KiB, committed %u KiB, largest free %u bytes, largest request %u bytes, map failures %u",
          live>>10,peak>>10,committed>>10,largest,request,maps);
    trace("# heap calls: alloc %llu free %llu realloc %llu; largest allocations (live, requested/capacity bytes):",
          (unsigned long long)allocations,(unsigned long long)frees,(unsigned long long)reallocations);
    for(unsigned i=0;i<TOP_ALLOCATIONS && top[i].address;i++)
        trace("# heap allocation %u: %08x requested %u capacity %u",i+1,top[i].address,top[i].size,top[i].capacity);
}
uint32_t heap_alloc(uint32_t size,int zero) {
    pthread_mutex_lock(&heap_lock); ++heap_alloc_calls;
    if(size>heap_max_request) heap_max_request=size;
    uint32_t p=heap_alloc_locked(size),cap=p?block_capacity(p-HDR):0;
    pthread_mutex_unlock(&heap_lock);
    if(p) { if(zero) memset(G(p),0,cap); }
    else { trace("# heap allocation failed: request %u bytes",size); heap_trace_stats(); }
    return p;
}
void heap_free(uint32_t p) {
    pthread_mutex_lock(&heap_lock); ++heap_free_calls; heap_free_locked(p); pthread_mutex_unlock(&heap_lock);
}
uint32_t heap_size(uint32_t p) {
    pthread_mutex_lock(&heap_lock);
    uint32_t size=heap_valid(p)?rd32(p-HDR):UINT32_MAX;
    pthread_mutex_unlock(&heap_lock); return size;
}
uint32_t heap_realloc(uint32_t p,uint32_t size,int zero) {
    if(!p) return heap_alloc(size,zero);
    pthread_mutex_lock(&heap_lock); ++heap_realloc_calls;
    if(size>heap_max_request) heap_max_request=size;
    if(!heap_valid(p) || size>GUEST_HEAP_SIZE-HDR) { pthread_mutex_unlock(&heap_lock); return 0; }
    uint32_t b=p-HDR,old=rd32(b),cap=block_capacity(b),need=((size?size:1)+15u)&~15u;
    int in_place=need<=cap;
    if(!in_place) {
        uint32_t next=b+HDR+cap;
        if(next<HEAP_END && rd32(next+4)==0) {
            uint32_t combined=cap+HDR+block_capacity(next);
            if(combined>=need && heap_prepare(b,combined,need)) {
                heap_unlink(next); wr32(b+8,combined); heap_fix_next(b); in_place=1;
            }
        }
    }
    if(in_place) {
        heap_split(b,need); wr32(b,size);
        heap_live=heap_live-cap+block_capacity(b);
        if(heap_live>heap_peak) heap_peak=heap_live;
        pthread_mutex_unlock(&heap_lock);
        if(zero && size>old) memset(G(p+old),0,size-old);
        return p;
    }
    uint32_t q=heap_alloc_locked(size),new_cap=q?block_capacity(q-HDR):0;
    pthread_mutex_unlock(&heap_lock);
    if(!q) { trace("# heap realloc failed: request %u bytes",size); heap_trace_stats(); return 0; }
    /* The caller owns p throughout; unrelated allocations may proceed while copying. */
    memcpy(G(q),G(p),old<size?old:size);
    if(zero && new_cap>old) memset(G(q+old),0,new_cap-old);
    heap_free(p); return q;
}
uint32_t heap_used(void) {
    pthread_mutex_lock(&heap_lock); uint32_t live=heap_live; pthread_mutex_unlock(&heap_lock); return live;
}

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
    heap_trace_stats();
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
    const ApiEntry *tables[] = {win32_fileio, win32_threads, win32_kernel32, win32_user, win32_stubs, win32_openal, win32_gl, win32_window};
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