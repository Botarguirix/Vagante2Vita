#define main unused_stress_main
#define heap_profile_images unused_image_report
#define guest_intercept unused_intercept
#include "heap_test.c"
#undef guest_intercept
#undef heap_profile_images
#undef main
static GuestFn image_create,image_destroy;
static uint32_t active_object;
void guest_intercept(uint32_t a,GuestFn fn) {
    if(a==0x8181f0) image_create=fn;
    if(a==0x817ec0) image_destroy=fn;
}
#define NOOP(a) void f_##a(Cpu *c) {(void)c;}
NOOP(008ccfa2) NOOP(008d9f35) NOOP(009a92b6) NOOP(008cd56c)
NOOP(008e27a9) NOOP(008e279e) NOOP(008ef765)
NOOP(00817e90) NOOP(0074b350) NOOP(00818340) NOOP(008184e0)
NOOP(00818500) NOOP(008169e0)
NOOP(00550190) NOOP(005c1570) NOOP(00550a00) NOOP(005c23b0)
NOOP(005c9840) NOOP(006a6810) NOOP(00707e50)
void platform_sleep_ms(uint32_t ms) {(void)ms;abort();}
void f_008181f0(Cpu *c) {
    uint32_t o=c->ecx.d;
    uint32_t p=heap_alloc(16384,0);assert(p);
    wr32(o,64);wr32(o+4,64);wr32(o+8,p);wr32(o+12,p+16384);wr32(o+16,p+16384);
    c->ecx.d=0xdeadbeef; /* wrapper must save this before delegation */
    c->eax.d=0x12345678;
}
void f_00817ec0(Cpu *c) {heap_free(rd32(c->ecx.d+8));wr32(c->ecx.d+8,0);}
int main(void) {
    guest_base=calloc(1,GUEST_WINDOW);assert(guest_base);
    heap_profile_init();assert(image_create&&image_destroy);
    active_object=heap_alloc(32,1);
    Cpu c={0};c.ecx.d=active_object;image_create(&c);assert(c.eax.d==0x12345678);
    heap_profile_decoded(0,1);heap_profile_decoded(1,1);heap_profile_decoded(1,0);
    heap_profile_dump("image-live");
    assert(strstr(messages,"rgba_bytes=16384 vector_capacity=16384 images=1"));
    assert(strstr(messages,"eager_decodes=1 demand_decodes=1"));
    c.ecx.d=active_object;image_destroy(&c);
    message_length=0;messages[0]=0;
    heap_profile_dump("image-destroyed");
    assert(strstr(messages,"rgba_bytes=0 vector_capacity=0 images=0"));
    heap_free(active_object);assert(heap_used()==0);
    trace_close();free(guest_base);
    puts("image profile: RGBA bytes, successful eager/demand, destructor, ABI preserved OK");
}
