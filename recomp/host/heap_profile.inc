/* Included by support.c: shares heap_lock; metadata stays OUTSIDE the guest arena.
 * Disabled by default. Allocation provenance is retained across free/realloc/reuse.
 * Hash load <= 1/2, tombstones reused; host allocation failure is reported explicitly. */
typedef struct { uint32_t p,origin,caller; } ProfileBlock;
static ProfileBlock *profile_blocks;
static size_t profile_slots,profile_live,profile_occupied;
static uint64_t profile_lost;
static __thread uint32_t profile_origin,profile_caller;
static pthread_once_t profile_once=PTHREAD_ONCE_INIT;
static int profile_mode;
static void profile_configure(void) {
    const char *v=getenv("RECOMP_HEAP_PROFILE");
    profile_mode=v && (!strcmp(v,"1") || !strcmp(v,"2")) ? v[0]-'0':0;
}
int heap_profile_mode(void) { pthread_once(&profile_once,profile_configure); return profile_mode; }
uint32_t heap_profile_scope(uint32_t origin) {
    uint32_t old=profile_origin;
    if(heap_profile_mode()) profile_origin=origin;
    return old;
}
static size_t profile_hash(uint32_t p,size_t n) { return ((p>>4)*2654435761u)&(n-1); }
static ProfileBlock *profile_find(uint32_t p) {
    if(!profile_slots) return NULL;
    size_t i=profile_hash(p,profile_slots);
    while(profile_blocks[i].p) {
        if(profile_blocks[i].p==p) return profile_blocks+i;
        i=(i+1)&(profile_slots-1);
    }
    return NULL;
}
static int profile_grow(void) {
    size_t n=profile_slots ? (profile_live*4<profile_slots?profile_slots:profile_slots*2):4096;
    ProfileBlock *next=calloc(n,sizeof *next);
    if(!next) { ++profile_lost; return 0; }
    for(size_t k=0;k<profile_slots;k++) if(profile_blocks[k].p>1) {
        size_t i=profile_hash(profile_blocks[k].p,n);
        while(next[i].p) i=(i+1)&(n-1);
        next[i]=profile_blocks[k];
    }
    free(profile_blocks);profile_blocks=next;profile_slots=n;profile_occupied=profile_live;
    return 1;
}
static void profile_put(uint32_t p) {
    if(!heap_profile_mode()) return;
    ProfileBlock *old=profile_find(p);
    if(old) { /* in-place realloc: attribute the resized block to its current caller */
        old->origin=profile_origin;old->caller=profile_caller;return;
    }
    if((profile_occupied+1)*2>=profile_slots && !profile_grow()) return;
    size_t i=profile_hash(p,profile_slots),tomb=SIZE_MAX;
    while(profile_blocks[i].p) {
        if(profile_blocks[i].p==1 && tomb==SIZE_MAX) tomb=i;
        i=(i+1)&(profile_slots-1);
    }
    if(tomb!=SIZE_MAX) i=tomb; else profile_occupied++;
    profile_blocks[i]=(ProfileBlock){p,profile_origin,profile_caller};profile_live++;
}
static void profile_remove(uint32_t p) {
    if(!heap_profile_mode()) return;
    ProfileBlock *e=profile_find(p);
    if(e) { e->p=1;--profile_live; }
}
typedef struct { uint32_t origin,caller;uint64_t bytes,requested,blocks; } ProfileGroup;
static int profile_by_origin(const void *a,const void *b) {
    const ProfileGroup *x=a,*y=b;return (x->origin>y->origin)-(x->origin<y->origin);
}
static int profile_by_bytes(const void *a,const void *b) {
    const ProfileGroup *x=a,*y=b;return (x->bytes<y->bytes)-(x->bytes>y->bytes);
}
static pthread_mutex_t profile_dump_lock=PTHREAD_MUTEX_INITIALIZER;
void heap_profile_dump(const char *reason) {
    if(!heap_profile_mode()) return;
    pthread_mutex_lock(&profile_dump_lock);
    pthread_mutex_lock(&heap_lock);
    size_t n=0;
    ProfileGroup *rows=calloc(profile_live+1,sizeof *rows);
    uint64_t free_bytes=0,live=heap_live,hist[4]={0},counts[4]={0},headers=0;
    uint32_t largest=0;
    int complete=rows!=NULL;
    if(heap_initialized) for(uint32_t b=GUEST_HEAP_BASE;b<HEAP_END;) {
        uint32_t cap=rd32(b+8),magic=rd32(b+4);
        if(cap<16 || (cap&15) || cap>HEAP_END-b-16) { complete=0;break; }
        headers+=16;
        if(!magic) { free_bytes+=cap;if(cap>largest) largest=cap; }
        else if(magic==MAGIC) {
            unsigned h=cap<4096?0:cap<65536?1:cap<1048576?2:3;
            hist[h]+=cap;counts[h]++;
            ProfileBlock *e=profile_find(b+16);
            if(rows && n<=profile_live) rows[n++]=(ProfileGroup){e?e->origin:0,e?e->caller:0,cap,rd32(b),1};
            else complete=0;
        } else { complete=0;break; }
        b+=16+cap;
    } else {free_bytes=GUEST_HEAP_SIZE-16;largest=(uint32_t)free_bytes;headers=16;}
    size_t metadata=profile_slots*sizeof(ProfileBlock);
    uint64_t lost=profile_lost;
    pthread_mutex_unlock(&heap_lock);
    if(rows) qsort(rows,n,sizeof *rows,profile_by_origin);
    size_t groups=0;
    for(size_t i=0;i<n;i++) {
        if(groups && rows[groups-1].origin==rows[i].origin) {
            rows[groups-1].bytes+=rows[i].bytes;rows[groups-1].requested+=rows[i].requested;rows[groups-1].blocks++;
        } else rows[groups++]=rows[i];
    }
    if(rows) qsort(rows,groups,sizeof *rows,profile_by_bytes);
    trace("# HEAP_PROFILE BEGIN reason=%s us=%llu window=%u arena=%u live=%llu free=%llu largest=%u headers=%llu complete=%d lost=%llu metadata_host=%llu",
        reason,(unsigned long long)platform_ticks_us(),GUEST_WINDOW,GUEST_HEAP_SIZE,
        (unsigned long long)live,(unsigned long long)free_bytes,largest,(unsigned long long)headers,
        complete,(unsigned long long)lost,(unsigned long long)metadata);
    for(unsigned h=0;h<4;h++) trace("# HEAP_PROFILE HIST bucket=%u bytes=%llu blocks=%llu",h,(unsigned long long)hist[h],(unsigned long long)counts[h]);
    for(size_t i=0;i<groups && i<30;i++) trace("# HEAP_PROFILE ORIGIN rank=%u addr=%08x bytes=%llu requested=%llu blocks=%llu heap_return_example=%08x",
        (unsigned)i+1,rows[i].origin,(unsigned long long)rows[i].bytes,(unsigned long long)rows[i].requested,
        (unsigned long long)rows[i].blocks,rows[i].caller);
    free(rows);
    heap_profile_images();
    trace("# HEAP_PROFILE END reason=%s groups=%llu",reason,(unsigned long long)groups);
    pthread_mutex_unlock(&profile_dump_lock);
}

/* sf::Image vectors >=4 KiB are aligned by MSVC with a raw pointer at pixels-4. */
int heap_profile_backed(uint32_t p,uint32_t bytes) {
    int valid=0;
    pthread_mutex_lock(&heap_lock);
    uint32_t raw=p;
    if(p>=GUEST_HEAP_BASE+20 && p<heap_committed && !profile_find(p)) raw=rd32(p-4);
    if(profile_find(raw) && raw<=p && p-raw<=35) {
        uint32_t cap=rd32(raw-8);
        valid=p-raw<=cap && bytes<=cap-(p-raw);
    }
    pthread_mutex_unlock(&heap_lock);
    return valid;
}
