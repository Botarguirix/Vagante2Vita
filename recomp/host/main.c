/* Host for the recompiled vagante.exe: loads the user's executable, fills its
 * IAT with host implementations, builds the thread environment and runs the CRT
 * entry point. Every Win32 call is written to host-trace.log; the run stops at the
 * first unimplemented import or guest_fault and reports how far it got.
 *
 * usage: vagante_host <vagante.exe> [data directory]
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "win32.h"

#ifdef _WIN32
#include <windows.h>

/* A host access violation inside the guest window is a guest memory fault: report
   the guest address and the last guest functions entered, then stop. */
static LONG WINAPI guest_access_violation(EXCEPTION_POINTERS *info) {
    if (info->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;
    uintptr_t host = (uintptr_t)info->ExceptionRecord->ExceptionInformation[1];
    int write = (int)info->ExceptionRecord->ExceptionInformation[0];
    if (host < (uintptr_t)guest_base || host >= (uintptr_t)guest_base + ((uintptr_t)1 << 32))
        return EXCEPTION_CONTINUE_SEARCH;
    uint32_t guest = (uint32_t)(host - (uintptr_t)guest_base);
    fprintf(stderr, "guest memory fault: %s of unmapped guest address %08x\n", write ? "write" : "read", guest);
    trace("# guest memory fault: %s %08x; last guest functions entered (oldest first):",
          write ? "write" : "read", guest);
    for (unsigned k = 0; k < 64; k++) {
        uint32_t f = guest_trace_ring[(guest_trace_pos + k) & 63];
        if (f)
            trace("#   %08x", f);
    }
    trace_summary();
    fflush(NULL);
    ExitProcess(0xFA17);
}
#endif

extern const GuestEntry t123_lifted[];
extern const unsigned t123_lifted_count;

static jmp_buf finished;
static uint32_t exit_code;
static const char *exit_reason = "returned";
static uint64_t started;
static pthread_t main_thread;

static int finish(void) {
    uint64_t elapsed_ms = (platform_ticks_us() - started) / 1000u;
    trace_summary();
    char summary[512];
    snprintf(summary, sizeof summary,
             "vagante host: %s, exit code %08x, %llu Win32 calls in %llu ms, heap %u KiB (trace: %s)",
             exit_reason, exit_code, import_calls, (unsigned long long)elapsed_ms, heap_used() / 1024,
             trace_path);
    trace("# %s", summary);
    trace_close();
    platform_finish(summary);
    return exit_code == 0 ? 0 : 1;
}

/* The game presents from its own render thread, so the run can end on any guest
   thread; only the main thread can unwind to main(). */
_Noreturn void host_exit(Cpu *c, uint32_t code, const char *why) {
    (void)c;
    exit_code = code;
    exit_reason = why;
    trace("# exit: %s (code %08x)", why, code);
    if (pthread_equal(pthread_self(), main_thread))
        longjmp(finished, 1);
    exit(finish());
}

static void setup_thread_environment(Cpu *c) {
    guest_map(GUEST_STACK_BASE, GUEST_STACK_SIZE);
    guest_map(GUEST_TIB, 0x2000); /* TIB + PEB */
    memset(G(GUEST_TIB), 0, 0x2000);
    wr32(GUEST_TIB + 0x00, 0xFFFFFFFFu);                         /* SEH chain end */
    wr32(GUEST_TIB + 0x04, GUEST_STACK_BASE + GUEST_STACK_SIZE); /* stack base */
    wr32(GUEST_TIB + 0x08, GUEST_STACK_BASE);                    /* stack limit */
    wr32(GUEST_TIB + 0x18, GUEST_TIB);                           /* self */
    wr32(GUEST_TIB + 0x20, 0x1000);                              /* process id */
    wr32(GUEST_TIB + 0x24, 0x1004);                              /* thread id */
    wr32(GUEST_TIB + 0x30, GUEST_PEB);
    wr32(GUEST_PEB + 0x08, guest_image_base);                    /* ImageBaseAddress */
    wr32(GUEST_PEB + 0x10, GUEST_PEB + 0x400);                   /* ProcessParameters (zeroed) */
    tls_init_thread(GUEST_TIB);
    memset(c, 0, sizeof *c);
    x87_reset(c);
    x87_set_cw(c, 0x027F); /* the MSVC process default: PC=53, round to nearest */
    c->fs_base = GUEST_TIB;
    c->esp.d = GUEST_STACK_BASE + GUEST_STACK_SIZE - 16;
}

#ifdef __vita__
/* Deep guest call chains run on the host stack. */
unsigned int sceUserMainThreadStackSize = 8 * 1024 * 1024;
#define DEFAULT_DATA_DIR "ux0:data/Vagante"
#else
#define DEFAULT_DATA_DIR "."
#endif

int main(int argc, char **argv) {
    char exe[1100];
    snprintf(host_data_dir, sizeof host_data_dir, "%s", argc > 2 ? argv[2] : DEFAULT_DATA_DIR);
    snprintf(exe, sizeof exe, "%s", argc > 1 ? argv[1] : DEFAULT_DATA_DIR "/vagante.exe");
#ifdef __vita__
    snprintf(trace_path, sizeof trace_path, "%s/recomp-trace.log", host_data_dir);
#endif
    trace("# vagante recompiled host, image %s", exe);
    char memory[160];
    platform_memory_report(memory, sizeof memory);
    trace("# before the guest window: %s", memory);
    if (guest_mem_init()) {
        char why[320];
        snprintf(why, sizeof why, "cannot allocate the %u MiB guest memory window (error %08x; %s)",
                 GUEST_WINDOW >> 20, (unsigned)guest_mem_error, memory);
        platform_finish(why);
        return 2;
    }
    if (guest_load_pe(exe)) {
        char why[1200];
        snprintf(why, sizeof why, "cannot load %s (copy vagante.exe and data.vra there)", exe);
        platform_finish(why);
        return 2;
    }
    guest_register(t123_lifted, t123_lifted_count);
    crt_native_init();
    imports_init();
#ifdef _WIN32
    AddVectoredExceptionHandler(1, guest_access_violation);
#endif

    static Cpu cpu;
    setup_thread_environment(&cpu);
    uint32_t pe = guest_image_base + rd32(guest_image_base + 0x3C);
    uint32_t entry = guest_image_base + rd32(pe + 40);
    /* The generated code only matches vagante.exe Steam 1.10.1 (SHA-256 9af1d8cc...624c):
       its entry point is 0x8ccf79 and its image 0x7f9000 bytes. Any other build
       would run unrelated code, so refuse it with a clear message. */
    if (entry != 0x008ccf79u || guest_image_size != 0x007f9000u) {
        char why[300];
        snprintf(why, sizeof why,
                 "wrong vagante.exe: entry %08x, image %08x bytes; the recompiled code needs "
                 "Steam 1.10.1 (entry 008ccf79, SHA-256 9af1d8cc...624c)", entry, guest_image_size);
        platform_finish(why);
        return 2;
    }
    trace("# entry %08x, data directory %s", entry, host_data_dir);

    jmp_buf fault;
    guest_fault_trap = &fault;
    started = platform_ticks_us();
    main_thread = pthread_self();
    if (setjmp(finished) == 0) {
        if (setjmp(fault) == 0) {
            push32(&cpu, GUEST_RETURN_SENTINEL);
            guest_call(&cpu, entry);
            exit_code = cpu.eax.d;
        } else {
            trace("# %s", guest_fault_text);
            exit_reason = guest_fault_text;
            exit_code = 0xFA17;
        }
    }
    return finish();
}
