/* Test-only load endpoint. Linked with --wrap=guest_intercept, never shipped. */
#include "win32.h"
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#endif
static GuestFn finish_initialization;
void __real_guest_intercept(uint32_t,GuestFn);
static void load_complete(Cpu *c) {
    finish_initialization(c);
    unsigned long long us=platform_ticks_us();
    fprintf(stderr,"FILEIO_LOAD_COMPLETE us=%llu\n",us);
    fflush(stderr);
    /* Deliberately skip fclose/atexit: tests the same buffered-log failure mode. */
#ifdef _WIN32
    TerminateProcess(GetCurrentProcess(),0); /* no MinGW CRT exit flush */
    abort();
#else
    _Exit(0);
#endif
}
void __wrap_guest_intercept(uint32_t address,GuestFn fn) {
    if(address==0x0073c6c0u) { finish_initialization=fn; fn=load_complete; }
    __real_guest_intercept(address,fn);
}
