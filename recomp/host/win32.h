/* Host implementation of the Win32 surface vagante.exe imports.
 *
 * Every import is a GuestFn reached through a fake address written into the IAT.
 * Implementations read stdcall arguments from the guest stack and return with
 * ret_stdcall, which pops the return address and the arguments like the callee
 * would. Unimplemented imports stop the program with their name.
 */
#ifndef T123_WIN32_H
#define T123_WIN32_H

#include "guest.h"
#include <stdio.h>

#ifndef _WIN32
#include <strings.h>
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#endif

/* Guest memory map, identical on PC and Vita. Everything the guest can dereference
 * lies below GUEST_WINDOW (128 MiB), so on the Vita it is one memory block:
 *   0x00110000 stack (2 MiB)      0x00400000 image (vagante.exe, to 0x00BF9000)
 *   0x00C00000 TIB, PEB, thread TIBs   0x01000000 thread stacks   0x01800000 heap (to the
 *   VirtualAlloc area)   GUEST_WINDOW - 8 MiB: VirtualAlloc (8 MiB)
 * Fake call targets and module handles sit above the window: they are compared
 * and called, never read. */
enum {
    GUEST_STACK_BASE = 0x00110000u, GUEST_STACK_SIZE = 0x00200000u,
    GUEST_TIB = 0x00C00000u, GUEST_PEB = 0x00C01000u,
    GUEST_HEAP_BASE = 0x01800000u, GUEST_HEAP_SIZE = GUEST_WINDOW - 0x00800000u - 0x01800000u,
    GUEST_VIRTUAL_BASE = GUEST_WINDOW - 0x00800000u, GUEST_VIRTUAL_SIZE = 0x00800000u, /* unused by the game */
    GUEST_FAKE_BASE = 0x7F000000u,
    FAKE_MODULE_KERNEL32 = 0x7C800000u, FAKE_MODULE_OTHER = 0x7C900000u,
};
_Static_assert(GUEST_VIRTUAL_BASE + GUEST_VIRTUAL_SIZE <= GUEST_WINDOW, "guest map exceeds the window");

uint32_t heap_used(void);
/* Implicit TLS (__declspec(thread), MSVC thread-safe statics): fs:[0x2C] points to an array
   at TIB+0xF10 whose slot *AddressOfIndex holds this thread's copy of the .tls template.
   The TlsAlloc slots stay at TIB+0xE10, as in a real TEB. */
void tls_init_thread(uint32_t tib);
uint32_t virtual_used(void);
extern char trace_path[256];
/* Platform hooks (platform_pc.c / recomp/vita/platform_vita.c). */
void platform_log(const char *line);   /* mirrors trace lines (Vita: on-screen log) */
uint64_t platform_ticks_us(void);      /* monotonic microseconds */
void platform_sleep_ms(uint32_t ms);
/* Texture storage (zeroed): host heap on PC, a CDRAM pool on the Vita. */
void *platform_texture_alloc(size_t bytes);
void platform_texture_free(void *p);
#ifdef __vita__
void texture_pool_reserve(void);      /* before vitaGL takes the remaining memory */
void platform_display_taken(void);    /* the renderer owns the screen: stop the text log */
#endif
void platform_finish(const char *summary);
void platform_memory_report(char *out, size_t size);

#define ARG(c, n) rd32((c)->esp.d + 4u + 4u * (n))
#define STR(a) ((char *)G(a))

static inline void ret_stdcall(Cpu *c, uint32_t value, unsigned nargs) {
    c->eax.d = value;
    c->esp.d += 4u + 4u * nargs;
}
static inline void ret_cdecl(Cpu *c, uint32_t value) {
    c->eax.d = value;
    c->esp.d += 4u;
}

/* -- registry ---------------------------------------------------------------------- */
typedef struct { const char *name; GuestFn fn; } ApiEntry;
extern const ApiEntry win32_threads[];
extern const ApiEntry win32_kernel32[];
extern const ApiEntry win32_user[];   /* user.c: USER32, GDI32, WINMM, OLE, IMM, VERSION */
extern const ApiEntry win32_gl[]; /* gl.c: OpenGL 1.1, guest stdcall */
extern const ApiEntry win32_openal[]; /* openal.c: OpenAL Soft, guest cdecl */
extern const ApiEntry win32_window[]; /* window.c: USER32 windows, GDI32, WGL on SDL2 */
extern const ApiEntry win32_stubs[];  /* stubs.c: Steam, networking, crypto, DirectInput */
void crt_native_init(void);

/* text.c: code pages. CP_UTF8 is UTF-8; every other code page is treated as Windows-1252. */
int mb_to_utf16(uint32_t cp, const uint8_t *src, int len, uint16_t *dst, int cap);
int utf16_to_mb(uint32_t cp, const uint16_t *src, int len, uint8_t *dst, int cap, int *defaulted);

int imports_init(void);                     /* fills the IAT with fake addresses */
uint32_t api_address(const char *name);     /* fake address for GetProcAddress, 0 if absent */
const char *api_name(uint32_t fake);        /* for traces */

/* -- support ----------------------------------------------------------------------- */
void trace(const char *fmt, ...);
void trace_summary(void);
void trace_close(void); /* before the process ends: the Vita does not flush on exit */
extern unsigned long long import_calls;
_Noreturn void host_exit(Cpu *c, uint32_t code, const char *why);

uint32_t heap_alloc(uint32_t size, int zero);
void heap_free(uint32_t p);
uint32_t heap_size(uint32_t p);
uint32_t heap_realloc(uint32_t p, uint32_t size, int zero);
uint32_t guest_strdup(const char *s);
uint32_t virtual_alloc(uint32_t address, uint32_t size);

void path_to_host(const char *guest_path, char *out, size_t size);
extern char host_data_dir[1024];
extern const char *guest_exe_path; /* the path the game sees for itself */

uint32_t handle_new(int type, void *host, uint32_t extra);
void *handle_get(uint32_t h, int type);
void handle_close(uint32_t h);
enum { H_FILE = 1, H_EVENT, H_MUTEX, H_THREAD, H_FIND, H_STD };

extern uint32_t last_error;

#endif

extern uint64_t texture_live, texture_peak;
void texture_census(void);
void com_summary(void);
void file_close(FILE *f); /* kernel32.c: keeps idle read-only files for reuse */
