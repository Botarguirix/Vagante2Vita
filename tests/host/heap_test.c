/* Same driver for the previous heap and TLSF; no game data. */
#include "win32.h"
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif
uint8_t *guest_base;
_Thread_local GuestThread guest_thread_state;
uint32_t guest_image_base;
const ApiEntry win32_fileio[]={{NULL,NULL}},win32_threads[]={{NULL,NULL}}, win32_kernel32[]={{NULL,NULL}},
    win32_user[]={{NULL,NULL}}, win32_stubs[]={{NULL,NULL}}, win32_openal[]={{NULL,NULL}},
    win32_gl[]={{NULL,NULL}}, win32_window[]={{NULL,NULL}};
void guest_intercept(uint32_t address,GuestFn fn) { (void)address;(void)fn; abort(); }
_Noreturn void host_exit(Cpu *c,uint32_t code,const char *why) { (void)c;(void)code;(void)why; abort(); }
void heap_profile_images(void) {} /* standalone heap fixture has no SFML objects */
void file_close(FILE *f) { fclose(f); }
static int fail_map;
static char messages[32768];
static size_t message_length;
int guest_map(uint32_t a,uint32_t n) { return fail_map || a>GUEST_WINDOW || n>GUEST_WINDOW-a ? -1:0; }
void platform_log(const char *s) {
    size_t n=strlen(s);
    if(message_length+n+2<sizeof messages) {
        memcpy(messages+message_length,s,n); message_length+=n;
        messages[message_length++]='\n'; messages[message_length]=0;
    }
}
uint64_t platform_ticks_us(void) {
#ifdef _WIN32
    LARGE_INTEGER f,t; QueryPerformanceFrequency(&f); QueryPerformanceCounter(&t);
    return (uint64_t)((double)t.QuadPart*1000000.0/(double)f.QuadPart);
#else
    struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
    return (uint64_t)t.tv_sec*1000000u+t.tv_nsec/1000u;
#endif
}
static uint32_t random32(uint32_t *s) { uint32_t x=*s; x^=x<<13; x^=x>>17; x^=x<<5; return *s=x; }
enum { PAIRS=1000000, THREADS=4, HOLES=512 };
static pthread_barrier_t start_barrier;
static void *bench_worker(void *arg) {
    (void)arg; pthread_barrier_wait(&start_barrier);
    for(unsigned i=0;i<PAIRS/THREADS;i++) { uint32_t p=heap_alloc(16384,1); assert(p); heap_free(p); }
    return NULL;
}
static void benchmark(const char *mode) {
    uint32_t guards[HOLES],holes[HOLES];
    if(strstr(mode,"fragmented")) {
        for(unsigned i=0;i<HOLES;i++) {
            holes[i]=heap_alloc(8192+16*(i%16),0); guards[i]=heap_alloc(16,0);
            assert(holes[i]&&guards[i]);
        }
        for(unsigned i=0;i<HOLES;i++) heap_free(holes[i]);
    }
    unsigned size=!strcmp(mode,"simple")?64:16384;
    for(unsigned i=0;i<4096;i++) { uint32_t p=heap_alloc(size,1); assert(p); heap_free(p); }
    uint64_t begin,end;
    if(!strncmp(mode,"threads",7)) {
        pthread_t threads[THREADS]; pthread_barrier_init(&start_barrier,NULL,THREADS+1);
        for(unsigned i=0;i<THREADS;i++) assert(!pthread_create(threads+i,NULL,bench_worker,NULL));
        begin=platform_ticks_us(); pthread_barrier_wait(&start_barrier);
        for(unsigned i=0;i<THREADS;i++) assert(!pthread_join(threads[i],NULL));
        end=platform_ticks_us(); pthread_barrier_destroy(&start_barrier);
    } else {
        begin=platform_ticks_us();
        for(unsigned i=0;i<PAIRS;i++) { uint32_t p=heap_alloc(size,0); assert(p); heap_free(p); }
        end=platform_ticks_us();
    }
    if(strstr(mode,"fragmented")) for(unsigned i=0;i<HOLES;i++) heap_free(guards[i]);
    printf("bench %s pairs=%u us=%llu\n",mode,PAIRS,(unsigned long long)(end-begin));
}

/* Allocation runs outside this independent range-registry lock: all four
   threads enter the allocator concurrently. During free/realloc we hold this
   test lock to avoid reporting stale ranges after another thread reuses them. */
enum { SLOTS=96, ITERATIONS=18000 };
typedef struct { uint32_t p,size; unsigned char pattern; } Live;
static Live live[THREADS][SLOTS];
static pthread_mutex_t registry=PTHREAD_MUTEX_INITIALIZER;
static void check_pattern(const Live *b) { for(uint32_t i=0;i<b->size;i++) assert(rd8(b->p+i)==b->pattern); }
static void register_range(unsigned t,unsigned slot,uint32_t p,uint32_t size) {
    assert(p>=GUEST_HEAP_BASE+16 && !(p&15) && size<=GUEST_HEAP_BASE+GUEST_HEAP_SIZE-p);
    uint32_t end=p+(((size?size:1)+15u)&~15u);
    for(unsigned a=0;a<THREADS;a++) for(unsigned b=0;b<SLOTS;b++) {
        if((a==t&&b==slot)||!live[a][b].p) continue;
        uint32_t q=live[a][b].p,n=live[a][b].size,qend=q+(((n?n:1)+15u)&~15u);
        assert(end<=q || qend<=p);
    }
    live[t][slot].p=p; live[t][slot].size=size;
}
static void *stress_worker(void *arg) {
    unsigned t=(unsigned)(uintptr_t)arg; uint32_t rng=0x97813211u+t*0x1199;
    for(unsigned i=0;i<ITERATIONS;i++) {
        unsigned slot=random32(&rng)%SLOTS; Live *b=&live[t][slot];
        uint32_t size=random32(&rng)%32769; int zero=(random32(&rng)&3)==0;
        if(!b->p) {
            uint32_t p=heap_alloc(size,zero); assert(p && heap_size(p)==size);
            pthread_mutex_lock(&registry); register_range(t,slot,p,size); pthread_mutex_unlock(&registry);
            if(zero) for(uint32_t n=0;n<size;n++) assert(!rd8(p+n));
            b->pattern=(unsigned char)(random32(&rng)|1); memset(G(p),b->pattern,size);
        } else {
            check_pattern(b); pthread_mutex_lock(&registry);
            if(random32(&rng)&1) {
                uint32_t old=b->size,p=heap_realloc(b->p,size,zero); assert(p && heap_size(p)==size);
                for(uint32_t n=0;n<(old<size?old:size);n++) assert(rd8(p+n)==b->pattern);
                if(zero && size>old) for(uint32_t n=old;n<size;n++) assert(!rd8(p+n));
                register_range(t,slot,p,size); memset(G(p),b->pattern,size);
            } else { heap_free(b->p); b->p=0; }
            pthread_mutex_unlock(&registry);
        }
    }
    for(unsigned i=0;i<SLOTS;i++) if(live[t][i].p) {
        pthread_mutex_lock(&registry); check_pattern(&live[t][i]);
        heap_free(live[t][i].p); live[t][i].p=0; pthread_mutex_unlock(&registry);
    }
    return NULL;
}
static void stress(void) {
    fail_map=1; assert(!heap_alloc(16,0)); fail_map=0;
    assert(!heap_used() && heap_size(0)==UINT32_MAX);
    uint32_t a=heap_alloc(33,1); assert(a && heap_size(a)==33 && !(a&15));
    for(unsigned i=0;i<48;i++) assert(!rd8(a+i));
    memset(G(a),0x75,33);
    uint32_t b=heap_alloc(10000,0),c=heap_alloc(64,0); assert(b&&c);
    heap_free(b); assert(heap_realloc(a,4096,1)==a);
    for(unsigned i=0;i<33;i++) assert(rd8(a+i)==0x75);
    for(unsigned i=33;i<4096;i++) assert(!rd8(a+i));
    assert(heap_realloc(a,17,0)==a && heap_size(a)==17);
    assert(!heap_realloc(a,UINT32_MAX,0) && heap_size(a)==17 && rd8(a)==0x75);
    heap_free(a); heap_free(c); heap_free(c); heap_free(0); heap_free(GUEST_HEAP_BASE+1);
    assert(!heap_used() && heap_size(c)==UINT32_MAX);
    assert(!heap_alloc(UINT32_MAX,0) && !heap_alloc(GUEST_HEAP_SIZE,0));
    assert(!heap_realloc(GUEST_HEAP_BASE+1,20,0));
    a=heap_realloc(0,0,1); assert(a && !heap_size(a)); heap_free(a);
    /* Failed page commitment must leave both the old allocation and bins intact. */
    a=heap_alloc(64,0); memset(G(a),0x39,64); fail_map=1;
    assert(!heap_realloc(a,2u<<20,1) && heap_size(a)==64 && rd8(a)==0x39);
    fail_map=0; assert(heap_realloc(a,2u<<20,1)==a);
    for(unsigned i=0;i<64;i++) assert(rd8(a+i)==0x39);
    for(unsigned i=64;i<(2u<<20);i++) assert(!rd8(a+i));
    heap_free(a); assert(!heap_used());
    for(unsigned power=4;power<21;power++) for(int offset=-17;offset<=17;offset++) {
        uint32_t n=(1u<<power)+offset;
        if(n>GUEST_HEAP_SIZE-16) continue;
        a=heap_alloc(n,0); assert(a && heap_size(a)==n);
        memset(G(a),0x3c,n); b=heap_alloc(64,0); assert(b);
        c=heap_realloc(a,n+8193,1); assert(c);
        for(uint32_t i=0;i<n;i++) assert(rd8(c+i)==0x3c);
        for(uint32_t i=n;i<n+8193;i++) assert(!rd8(c+i));
        heap_free(c); heap_free(b);
    }
    pthread_t workers[THREADS];
    for(unsigned i=0;i<THREADS;i++) assert(!pthread_create(workers+i,NULL,stress_worker,(void*)(uintptr_t)i));
    for(unsigned i=0;i<THREADS;i++) assert(!pthread_join(workers[i],NULL));
    assert(!heap_used());
    a=heap_alloc(GUEST_HEAP_SIZE-16,0); assert(a);
    assert(!heap_alloc(1,0)); heap_free(a); assert(!heap_used());
    a=heap_alloc(64,0); memset(G(a),0x6d,64);
    b=heap_alloc(GUEST_HEAP_SIZE-2*16-64,0); assert(b);
    assert(!heap_realloc(a,128,1) && heap_size(a)==64);
    for(unsigned i=0;i<64;i++) assert(rd8(a+i)==0x6d);
    heap_free(b); assert(heap_realloc(a,128,1)==a); heap_free(a); assert(!heap_used());
    a=heap_alloc(8u<<20,0); b=heap_alloc(4u<<20,0); assert(a&&b);
    message_length=0; trace_summary();
    assert(strstr(messages,"largest free") && strstr(messages,"largest allocations"));
    heap_free(a); heap_free(b); assert(!heap_used());
    puts("heap stress: 4 threads x 18000 random operations; data, no overlaps, realloc, boundaries and live=0 OK");
}
int main(int argc,char **argv) {
    guest_base=calloc(1,GUEST_WINDOW); assert(guest_base);
    if(argc==3&&!strcmp(argv[1],"--bench")) benchmark(argv[2]); else stress();
    trace_close(); free(guest_base); return 0;
}
