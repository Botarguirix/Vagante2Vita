/* Real threads for the recompiled game (F2.1): every guest thread is a host thread
 * with its own Cpu, guest stack, TIB (fs:) and TLS slots. Critical sections, events
 * and waits map to pthread primitives. Portable: pthreads exist on the PC toolchain
 * (winpthreads) and in VitaSDK. */
#include "win32.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Guest layout for secondary threads: TIBs after the main TIB/PEB, stacks in the free
   range below the heap. */
enum { MAX_THREADS = 24, THREAD_TIB_BASE = 0x00C02000u, THREAD_STACK_BASE = 0x01000000u,
       THREAD_STACK_SIZE = 0x00040000u };
_Static_assert(THREAD_STACK_BASE + MAX_THREADS * THREAD_STACK_SIZE <= GUEST_HEAP_BASE, "thread stacks");

#define API(name) static void name(Cpu *c)

typedef struct {
    pthread_mutex_t m;
    pthread_cond_t cv;
    int manual, signaled;
} Event;

typedef struct {
    Event done;          /* signaled when the guest thread returns */
    uint32_t start, param, exit_code, tib;
    int exiting;
    unsigned index;
    pthread_t host;
    Cpu cpu;             /* not _Thread_local: shared by all threads on the Vita */
} Thread;

static pthread_mutex_t table_lock = PTHREAD_MUTEX_INITIALIZER;
static Thread *threads[MAX_THREADS];

static void event_init(Event *e, int manual, int signaled) {
    pthread_mutex_init(&e->m, NULL);
    pthread_cond_init(&e->cv, NULL);
    e->manual = manual;
    e->signaled = signaled;
}

static void event_set(Event *e) {
    pthread_mutex_lock(&e->m);
    e->signaled = 1;
    pthread_cond_broadcast(&e->cv);
    pthread_mutex_unlock(&e->m);
}

/* 0 = signaled (WAIT_OBJECT_0), 0x102 = WAIT_TIMEOUT */
static uint32_t event_wait(Event *e, uint32_t ms) {
    pthread_mutex_lock(&e->m);
    if (!e->signaled && ms != 0xFFFFFFFFu) {
        struct timespec until;
        clock_gettime(CLOCK_REALTIME, &until);
        until.tv_sec += ms / 1000;
        until.tv_nsec += (long)(ms % 1000) * 1000000L;
        if (until.tv_nsec >= 1000000000L) { until.tv_sec++; until.tv_nsec -= 1000000000L; }
        while (!e->signaled)
            if (pthread_cond_timedwait(&e->cv, &e->m, &until) == ETIMEDOUT)
                break;
    } else {
        while (!e->signaled)
            pthread_cond_wait(&e->cv, &e->m);
    }
    uint32_t r = e->signaled ? 0 : 0x102;
    if (e->signaled && !e->manual)
        e->signaled = 0;
    pthread_mutex_unlock(&e->m);
    return r;
}

/* -- critical sections, keyed by the guest CRITICAL_SECTION address --------------- */
enum { CS_SLOTS = 4096 };
static struct { uint32_t address; pthread_mutex_t m; } sections[CS_SLOTS];

static pthread_mutex_t *section(uint32_t address) {
    unsigned h = (address >> 2) % CS_SLOTS;
    pthread_mutex_lock(&table_lock);
    for (unsigned k = 0; k < CS_SLOTS; k++, h = (h + 1) % CS_SLOTS) {
        if (sections[h].address == address) break;
        if (!sections[h].address) {
            pthread_mutexattr_t attr;
            pthread_mutexattr_init(&attr);
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
            pthread_mutex_init(&sections[h].m, &attr);
            sections[h].address = address;
            break;
        }
    }
    pthread_mutex_unlock(&table_lock);
    return &sections[h].m;
}

API(InitializeCriticalSection) { (void)section(ARG(c, 0)); ret_stdcall(c, 0, 1); }
API(InitializeCriticalSectionAndSpinCount) { (void)section(ARG(c, 0)); ret_stdcall(c, 1, 2); }
API(DeleteCriticalSection) { ret_stdcall(c, 0, 1); }
API(EnterCriticalSection) { pthread_mutex_lock(section(ARG(c, 0))); ret_stdcall(c, 0, 1); }
API(LeaveCriticalSection) { pthread_mutex_unlock(section(ARG(c, 0))); ret_stdcall(c, 0, 1); }
API(TryEnterCriticalSection) { ret_stdcall(c, pthread_mutex_trylock(section(ARG(c, 0))) == 0, 1); }

/* -- events and waits --------------------------------------------------------------- */
API(CreateEventA) {
    Event *e = malloc(sizeof *e);
    event_init(e, (int)ARG(c, 1), (int)ARG(c, 2));
    uint32_t h = handle_new(H_EVENT, e, 0);
    trace("CreateEventA(manual=%u, initial=%u) = %08x", ARG(c, 1), ARG(c, 2), h);
    ret_stdcall(c, h, 4);
}
API(SetEvent) {
    Event *e = handle_get(ARG(c, 0), H_EVENT);
    if (e) event_set(e);
    ret_stdcall(c, e != NULL, 1);
}
API(WaitForSingleObject) {
    uint32_t h = ARG(c, 0), ms = ARG(c, 1), r = 0;
    Event *e = handle_get(h, H_EVENT);
    Thread *t = handle_get(h, H_THREAD);
    if (e) r = event_wait(e, ms);
    else if (t) {
        r = event_wait(&t->done, ms);
        if (!r) event_set(&t->done); /* a finished thread stays signaled */
    }
    ret_stdcall(c, r, 2);
}
API(CreateMutexA) {
    trace("CreateMutexA(%s)", ARG(c, 2) ? STR(ARG(c, 2)) : "NULL");
    last_error = 0;
    ret_stdcall(c, handle_new(H_MUTEX, NULL, 0), 3);
}

/* -- threads ------------------------------------------------------------------------- */
/* ExitThread on a secondary thread unwinds to thread_main through the fault trap; the
   thread-specific key says which guest thread is running. */
static pthread_key_t current_thread;
static pthread_once_t current_once = PTHREAD_ONCE_INIT;
static void make_current_key(void) { pthread_key_create(&current_thread, NULL); }

static void *thread_main(void *arg) {
    Thread *t = arg;
    pthread_once(&current_once, make_current_key);
    pthread_setspecific(current_thread, t);
    Cpu *c = &t->cpu;
    memset(c, 0, sizeof *c);
    x87_reset(c);
    x87_set_cw(c, 0x027F);
    c->fs_base = t->tib;
    uint32_t stack = THREAD_STACK_BASE + t->index * THREAD_STACK_SIZE;
    c->esp.d = stack + THREAD_STACK_SIZE - 16;
    jmp_buf trap;
    guest_fault_trap = &trap;
    if (setjmp(trap) == 0) {
        uint32_t param = t->param;
        t->exit_code = guest_invoke(c, t->start, 1, &param);
        trace("# thread %u (start %08x) returned %u", t->index, t->start, t->exit_code);
    } else if (t->exiting) {
        trace("# thread %u (start %08x) exited with %u", t->index, t->start, t->exit_code);
    } else {
        trace("# thread %u (start %08x): %s", t->index, t->start, guest_fault_text);
        t->exit_code = 0xFA17;
    }
    event_set(&t->done);
    pthread_mutex_lock(&table_lock);
    threads[t->index] = NULL; /* the slot (stack, TIB) can be reused */
    pthread_mutex_unlock(&table_lock);
    return NULL;
}

API(CreateThread) {
    uint32_t start = ARG(c, 2), param = ARG(c, 3), flags = ARG(c, 4);
    pthread_mutex_lock(&table_lock);
    unsigned index = 1;
    while (index < MAX_THREADS && threads[index]) index++;
    Thread *t = index < MAX_THREADS && !(flags & 4) ? calloc(1, sizeof *t) : NULL;
    if (t) threads[index] = t;
    pthread_mutex_unlock(&table_lock);
    if (index == MAX_THREADS || (flags & 4 /* CREATE_SUSPENDED */)) {
        trace("CreateThread(start=%08x, flags=%08x): not supported", start, flags);
        ret_stdcall(c, 0, 6);
        return;
    }
    event_init(&t->done, 1, 0);
    t->start = start;
    t->param = param;
    t->index = index;
    t->tib = THREAD_TIB_BASE + (index - 1) * 0x1000u;
    uint32_t stack = THREAD_STACK_BASE + index * THREAD_STACK_SIZE;
    guest_map(stack, THREAD_STACK_SIZE);
    guest_map(t->tib, 0x1000);
    memset(G(t->tib), 0, 0x1000);
    wr32(t->tib + 0x00, 0xFFFFFFFFu);
    wr32(t->tib + 0x04, stack + THREAD_STACK_SIZE);
    wr32(t->tib + 0x08, stack);
    wr32(t->tib + 0x18, t->tib);
    wr32(t->tib + 0x20, 0x1000);
    wr32(t->tib + 0x24, 0x1004 + 4 * index);
    tls_init_thread(t->tib);
    wr32(t->tib + 0x30, GUEST_PEB);
    uint32_t h = handle_new(H_THREAD, t, 0);
    if (ARG(c, 5))
        wr32(ARG(c, 5), 0x1004 + 4 * index);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    /* Nobody joins guest threads (their end is signalled through t->done): detached, an
       exited thread gives its stack back. On the Vita the 7th thread failed with EAGAIN. */
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
#ifdef __vita__
    /* host stack for nested guest calls; 2 MiB each left too little for the 11th thread
       after vitaGL took its pools (EAGAIN on hardware) */
    pthread_attr_setstacksize(&attr, 1u << 20);
#else
    pthread_attr_setstacksize(&attr, 8u << 20);
#endif
    int err = pthread_create(&t->host, &attr, thread_main, t);
    pthread_attr_destroy(&attr);
    if (err) {
        trace("CreateThread(start=%08x): pthread_create failed (%d)", start, err);
        pthread_mutex_lock(&table_lock);
        threads[index] = NULL;
        pthread_mutex_unlock(&table_lock);
        ret_stdcall(c, 0, 6);
        return;
    }
    trace("CreateThread(start=%08x, param=%08x) = %08x (guest thread %u)", start, param, h, index);
    ret_stdcall(c, h, 6);
}
API(GetExitCodeThread) {
    Thread *t = handle_get(ARG(c, 0), H_THREAD);
    uint32_t code = 259; /* STILL_ACTIVE */
    if (t && event_wait(&t->done, 0) == 0) { code = t->exit_code; event_set(&t->done); }
    wr32(ARG(c, 1), code);
    ret_stdcall(c, t != NULL, 2);
}
API(SetThreadPriority) { ret_stdcall(c, 1, 2); }
API(GetCurrentThreadId) { ret_stdcall(c, rd32(c->fs_base + 0x24), 0); }
API(Sleep) { platform_sleep_ms(ARG(c, 0)); ret_stdcall(c, 0, 1); }

static void exit_thread(Cpu *c, uint32_t code) {
    pthread_once(&current_once, make_current_key);
    Thread *t = pthread_getspecific(current_thread);
    if (!t)
        host_exit(c, code, "ExitThread on the main thread");
    t->exit_code = code;
    t->exiting = 1;
    longjmp(*guest_fault_trap, 1);
}
API(ExitThread) { exit_thread(c, ARG(c, 0)); }
API(FreeLibraryAndExitThread) { exit_thread(c, ARG(c, 1)); }

#define E(name) {#name, name}
const ApiEntry win32_threads[] = {
    E(InitializeCriticalSection), E(InitializeCriticalSectionAndSpinCount), E(DeleteCriticalSection),
    E(EnterCriticalSection), E(LeaveCriticalSection), E(TryEnterCriticalSection),
    E(CreateEventA), E(SetEvent), E(WaitForSingleObject), E(CreateMutexA),
    E(CreateThread), E(ExitThread), E(FreeLibraryAndExitThread), E(GetExitCodeThread), E(SetThreadPriority), E(GetCurrentThreadId), E(Sleep),
    {NULL, NULL},
};
