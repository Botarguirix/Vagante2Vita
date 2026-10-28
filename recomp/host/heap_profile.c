/* Measurement-only intercepts. Every wrapper executes its original lifted body.
 * Addresses/layout verified against PDB + the listed Ghidra functions; no resource
 * lifetime, decoded pixels, loading decision or guest return value is changed. */
#include "win32.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define ALLOC_HOOK(addr) \
    void f_##addr(Cpu *c); \
    static void alloc_##addr(Cpu *c) { \
        uint32_t old=heap_profile_scope(0); \
        heap_profile_scope(old?old:rd32(c->esp.d)); \
        f_##addr(c);heap_profile_scope(old); \
    }
ALLOC_HOOK(008ccfa2) /* operator new */
ALLOC_HOOK(008d9f35)
ALLOC_HOOK(009a92b6)
ALLOC_HOOK(008cd56c) /* operator new[] */
ALLOC_HOOK(008e27a9) /* malloc */
ALLOC_HOOK(008e279e) /* calloc */
ALLOC_HOOK(008ef765) /* realloc */

/* sf::Image: width +0, height +4, vector begin/end/capacity +8/+12/+16.
 * Confirmed by 00817e90, 008181f0, 00818340, 0074b350, 00817ec0. */
typedef struct { uint32_t object; } ImageRecord;
static ImageRecord *images;
static size_t image_count,image_slots;
static uint64_t eager_decodes,demand_decodes;
static pthread_mutex_t image_lock=PTHREAD_MUTEX_INITIALIZER;
static void image_observe(uint32_t object,int remove) {
    if(!object || object>GUEST_WINDOW-20) return;
    pthread_mutex_lock(&image_lock);
    size_t i=0;
    while(i<image_count && images[i].object!=object) i++;
    if(remove) {if(i<image_count) images[i]=images[--image_count];}
    else if(i==image_count) {
        if(image_count==image_slots) {
            size_t n=image_slots?image_slots*2:1024;
            ImageRecord *p=realloc(images,n*sizeof *p);
            if(p) {images=p;image_slots=n;}
        }
        if(image_count<image_slots) images[image_count++]=(ImageRecord){object};
    }
    pthread_mutex_unlock(&image_lock);
}
void heap_profile_decoded(int on_demand,int success) {
    if(!heap_profile_mode() || !success) return;
    pthread_mutex_lock(&image_lock);
    if(on_demand) demand_decodes++; else eager_decodes++;
    pthread_mutex_unlock(&image_lock);
}
void heap_profile_images(void) {
    uint64_t bytes=0,capacity=0,count=0,invalid=0;
    pthread_mutex_lock(&image_lock);
    for(size_t i=0;i<image_count;i++) {
        uint32_t o=images[i].object,p=rd32(o+8),end=rd32(o+12),cap=rd32(o+16);
        uint64_t rgba=(uint64_t)rd32(o)*rd32(o+4)*4;
        if(!p || !rgba) continue;
        if(end<p || cap<end || cap>=GUEST_WINDOW || rgba!=end-p || !heap_profile_backed(p,cap-p)) {invalid++;continue;}
        bytes+=rgba;capacity+=cap-p;count++;
    }
    uint64_t eager=eager_decodes,demand=demand_decodes;
    size_t objects=image_count;
    pthread_mutex_unlock(&image_lock);
    trace("# HEAP_PROFILE IMAGES rgba_bytes=%llu vector_capacity=%llu images=%llu observed_objects=%llu invalid_or_inflight=%llu eager_decodes=%llu demand_decodes=%llu",
        (unsigned long long)bytes,(unsigned long long)capacity,(unsigned long long)count,
        (unsigned long long)objects,(unsigned long long)invalid,(unsigned long long)eager,(unsigned long long)demand);
}
#define IMAGE_HOOK(addr) \
    void f_##addr(Cpu *c); \
    static void image_##addr(Cpu *c) {uint32_t o=c->ecx.d;f_##addr(c);image_observe(o,0);}
IMAGE_HOOK(00817e90) /* constructor */
IMAGE_HOOK(0074b350) /* assignment */
IMAGE_HOOK(008181f0) /* create(color) */
IMAGE_HOOK(00818340) /* create(pixels) */
IMAGE_HOOK(008184e0) /* loadFromFile */
IMAGE_HOOK(00818500) /* loadFromMemory */
void f_00817ec0(Cpu *c);
static void image_destruct(Cpu *c) {image_observe(c->ecx.d,1);f_00817ec0(c);}
void f_008169e0(Cpu *c);
static void image_from_texture(Cpu *c) {f_008169e0(c);image_observe(c->eax.d,0);}

#define WORLD_HOOK(addr,reason) \
    void f_##addr(Cpu *c); \
    static void world_##addr(Cpu *c) { \
        heap_profile_dump(reason "-before");f_##addr(c);heap_profile_dump(reason "-after"); \
    }
WORLD_HOOK(00550190,"Client-newGame")
WORLD_HOOK(005c1570,"GameEngine-newGame")
WORLD_HOOK(00550a00,"Client-loadNextLevel")
WORLD_HOOK(005c23b0,"GameEngine-loadNextLevel")
WORLD_HOOK(005c9840,"GameEngine-swapLevel")
/* Menu state is evidence for log-only navigation. PDB MainMenu/PlayerCustomizeMenu offsets. */
void f_006a6810(Cpu *c);
static void profile_menu_action(Cpu *c) {
    uint32_t o=c->ecx.d;
    trace("# HEAP_PROFILE MENU action=%u open=%u focus=%u cursor=%u intro=%u",ARG(c,0),rd8(o+0xe4),rd8(o+0xe5),rd32(o+0x128),rd32(o+0x15d0));
    f_006a6810(c);
}
void f_00707e50(Cpu *c);
static void profile_customize_step(Cpu *c) {
    uint32_t o=c->ecx.d;
    f_00707e50(c);
    static uint64_t last=UINT64_MAX;
    uint64_t state=rd8(o)|((uint64_t)rd8(o+1)<<8)|((uint64_t)rd8(o+2)<<16)|((uint64_t)rd8(o+0x18)<<24)|((uint64_t)rd8(o+0x28)<<32)|((uint64_t)rd32(o+0x2c)<<40);
    if(state!=last) {
        last=state;
        trace("# HEAP_PROFILE CUSTOMIZE hidden=%u open=%u focus=%u stopped=%u selecting=%u class=%u",rd8(o),rd8(o+1),rd8(o+2),rd8(o+0x18),rd8(o+0x28),rd32(o+0x2c));
    }
}
static void *profile_timer(void *arg) {
    (void)arg;
    for(;;) {platform_sleep_ms(30000);heap_profile_dump("periodic-30s");}
    return NULL;
}
void heap_profile_init(void) {
    if(!heap_profile_mode()) return;
#define A(addr) guest_intercept(0x##addr##u,alloc_##addr)
    A(008ccfa2);A(008d9f35);A(009a92b6);A(008cd56c);A(008e27a9);A(008e279e);A(008ef765);
#undef A
#define I(addr) guest_intercept(0x##addr##u,image_##addr)
    I(00817e90);I(0074b350);I(008181f0);I(00818340);I(008184e0);I(00818500);
#undef I
    guest_intercept(0x00817ec0u,image_destruct);
    guest_intercept(0x008169e0u,image_from_texture);
#define W(addr) guest_intercept(0x##addr##u,world_##addr)
    W(00550190);W(005c1570);W(00550a00);W(005c23b0);W(005c9840);
#undef W
    if(heap_profile_mode()==2) {
        pthread_t thread;
        int error=pthread_create(&thread,NULL,profile_timer,NULL);
        if(!error) pthread_detach(thread);else trace("# HEAP_PROFILE timer failure %d",error);
    }
    guest_intercept(0x006a6810u,profile_menu_action);
    guest_intercept(0x00707e50u,profile_customize_step);
    trace("# HEAP_PROFILE enabled mode=%d",heap_profile_mode());
}
