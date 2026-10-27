/* Write-through completion for guest writable FILE handles. The underlying
 * KERNEL32 open/write/close implementations and read-only reuse cache are kept.
 * This table must precede KERNEL32 in support.c's import lookup. */
#include "win32.h"
#include <pthread.h>
#include <string.h>
#define API(name) static void fileio_##name(Cpu *c)
enum { OPEN_A, OPEN_W, WRITE, CLOSE, API_COUNT, HANDLE_BASE=0x1000, HANDLE_SLOTS=4096 };
static GuestFn original[API_COUNT];
static pthread_once_t original_once=PTHREAD_ONCE_INIT;
static const char *names[API_COUNT]={"CreateFileA","CreateFileW","WriteFile","CloseHandle"};
static const unsigned nargs[API_COUNT]={7,7,5,1};
/* Same handle encoding as support.c: base + 4*slot. Keep FILE identity too, so
   recycling a handle for a non-file cannot inherit a previous writable flag. */
static struct { FILE *stream; int writable; } modes[HANDLE_SLOTS];
static pthread_mutex_t mode_lock=PTHREAD_MUTEX_INITIALIZER;
static void resolve_originals(void) {
    const ApiEntry *tables[]={win32_threads,win32_kernel32};
    for(unsigned t=0;t<sizeof tables/sizeof tables[0];t++)
        for(const ApiEntry *entry=tables[t];entry->name;entry++)
            for(unsigned i=0;i<API_COUNT;i++)
                if(!original[i] && !strcmp(entry->name,names[i])) original[i]=entry->fn;
}
static void delegate(Cpu *c,unsigned api) {
    pthread_once(&original_once,resolve_originals);
    if(original[api]) original[api](c);
    else { last_error=127; ret_stdcall(c,api<=OPEN_W?UINT32_MAX:0,nargs[api]); }
}
static unsigned slot_index(uint32_t handle) {
    if(handle<HANDLE_BASE || ((handle-HANDLE_BASE)&3)) return HANDLE_SLOTS;
    unsigned slot=(handle-HANDLE_BASE)/4;
    return slot<HANDLE_SLOTS?slot:HANDLE_SLOTS;
}
static void note_open(uint32_t handle,uint32_t access) {
    unsigned slot=slot_index(handle);
    if(slot==HANDLE_SLOTS) return;
    FILE *stream=handle_get(handle,H_FILE);
    pthread_mutex_lock(&mode_lock);
    modes[slot].stream=stream;
    /* This is the access bit that selects r+b/w+b in the existing A/W APIs. */
    modes[slot].writable=stream && (access&0x40000000u);
    pthread_mutex_unlock(&mode_lock);
}
API(CreateFileA) { uint32_t access=ARG(c,1); delegate(c,OPEN_A); note_open(c->eax.d,access); }
API(CreateFileW) { uint32_t access=ARG(c,1); delegate(c,OPEN_W); note_open(c->eax.d,access); }
API(CloseHandle) {
    unsigned slot=slot_index(ARG(c,0));
    if(slot<HANDLE_SLOTS) {
        /* Clear BEFORE closing releases the slot for another opener. */
        pthread_mutex_lock(&mode_lock);
        modes[slot].stream=NULL; modes[slot].writable=0;
        pthread_mutex_unlock(&mode_lock);
    }
    delegate(c,CLOSE);
}
API(WriteFile) {
    uint32_t handle=ARG(c,0);
    FILE *stream=handle_get(handle,H_FILE);
    unsigned slot=slot_index(handle);
    int writable=0;
    if(stream && slot<HANDLE_SLOTS) {
        pthread_mutex_lock(&mode_lock);
        writable=modes[slot].stream==stream && modes[slot].writable;
        pthread_mutex_unlock(&mode_lock);
    }
    delegate(c,WRITE);
    /* Never fflush(NULL), stdin/stdout, or a reused read-only stream. The mode
       lock is not held over fwrite/fflush; independent files stay independent.
       fflush transfers stdio buffers to the OS, not a disk/power-loss fsync. */
    /* At most one flush per second: on the Vita memory card each fflush costs ~0.4 s (73 log
       writes took 29 s), so the log is flushed close to real time without stalling the game. */
    static uint64_t last_flush;
    uint64_t now = platform_ticks_us();
    if(writable && now - last_flush < 1000000u) writable=0;
    else if(writable) last_flush=now;
    if(writable && fflush(stream)) {
        last_error=29; /* ERROR_WRITE_FAULT */
        c->eax.d=0;
    }
}
#define E(name) {#name,fileio_##name}
const ApiEntry win32_fileio[]={E(CreateFileA),E(CreateFileW),E(WriteFile),E(CloseHandle),{NULL,NULL}};
