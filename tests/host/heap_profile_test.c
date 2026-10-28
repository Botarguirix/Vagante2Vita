/* Reuse the real guest arena fixture, not the allocator implementation. */
#define main heap_stress_main
#include "heap_test.c"
#undef main
static uint32_t threaded_blocks[4][64];
static void *profile_worker(void *arg) {
    unsigned id=(unsigned)(uintptr_t)arg;
    heap_profile_scope(0x44440000u+id);
    for(unsigned i=0;i<64;i++) {
        threaded_blocks[id][i]=heap_alloc(128+16*id,0);
        assert(threaded_blocks[id][i]);
    }
    return NULL;
}
int main(void) {
    guest_base=calloc(1,GUEST_WINDOW);assert(guest_base);
    assert(heap_profile_mode()==1);
    heap_profile_scope(0x11111111);
    uint32_t a=heap_alloc(4000,0),b=heap_alloc(4096,0);
    heap_profile_scope(0x22222222);
    uint32_t d=heap_alloc(65536,0),e=heap_alloc(1048576,0);
    assert(a&&b&&d&&e);
    heap_profile_dump("fixture-live");
    heap_free(a);heap_free(b);
    heap_profile_scope(0x33333333);
    uint32_t q=heap_realloc(d,2000000,0);assert(q);
    heap_profile_dump("fixture-realloc");
    heap_free(q);heap_free(e);
    heap_profile_dump("fixture-empty");
    assert(heap_used()==0);
    assert(!heap_alloc(GUEST_HEAP_SIZE,0));
    pthread_t workers[4];
    for(unsigned i=0;i<4;i++) assert(!pthread_create(workers+i,NULL,profile_worker,(void *)(uintptr_t)i));
    for(unsigned i=0;i<4;i++) assert(!pthread_join(workers[i],NULL));
    heap_profile_dump("fixture-four-origins");
    for(unsigned i=0;i<4;i++) for(unsigned j=0;j<64;j++) heap_free(threaded_blocks[i][j]);
    assert(heap_used()==0);
    trace_close();
    free(guest_base);
    puts("heap profile: live origins, histograms, realloc, reuse, OOM, final zero OK");
    return 0;
}
