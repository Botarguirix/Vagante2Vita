/* Reuse the real guest arena fixture, not the allocator implementation. */
#define main heap_stress_main
#include "heap_test.c"
#undef main
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
    trace_close();
    free(guest_base);
    puts("heap profile: live origins, histograms, realloc, reuse, OOM, final zero OK");
    return 0;
}
