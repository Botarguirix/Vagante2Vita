/* On-demand image decoding for vagante.exe (the rama A RECON_LAZY_IMAGES design, as intercepts).
 *
 * The original decodes all ~2,400 PNGs while loading and keeps their RGBA pixels: ~250 MiB,
 * more than the Vita can hold. Here ResourceManager::loadImage only records the image (and
 * pins its compressed PNG in the archive); the first request for its texture runs the original
 * loadImage, which decodes it as usual. Images that loading tasks look up directly in the
 * image map (process-image jobs, repeat flags, VFonts) are decoded eagerly, as are palette-swap
 * sources (processPaletteSwap decodes its texture first).
 *
 * Calling conventions (LTCG, checked in the disassembly; cdecl, caller pops the stack):
 *   queueTask        0x74fe60  ecx = TaskType, [esp+4] std::string path (by value), ...
 *   loadImage        0x74ab70  cl = smooth, dl = makeMask, [esp+4] makeOutline, [esp+8] path
 *   getTexture       0x74cb20  cl = bool, [esp+4] std::string name
 *   getTextureMask   0x74cdf0, getTextureOutlineMask 0x74d0b0  [esp+4] std::string name
 *   getTextureAltPalette 0x74d370, processPaletteSwap 0x751930  ecx = const std::string &texture
 * MSVC std::string: 16-byte buffer (inline when capacity < 16) or pointer, size +16, capacity +20. */
#include "win32.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

enum { QUEUE_TASK = 0x0074fe60u, LOAD_IMAGE = 0x0074ab70u, GET_TEXTURE = 0x0074cb20u,
       GET_TEXTURE_MASK = 0x0074cdf0u, GET_TEXTURE_OUTLINE = 0x0074d0b0u,
       GET_TEXTURE_PALETTE = 0x0074d370u, PROCESS_PALETTE_SWAP = 0x00751930u };
void f_0074fe60(Cpu *c);
void f_0074ab70(Cpu *c);
void f_0074cb20(Cpu *c);
void f_0074cdf0(Cpu *c);
void f_0074d0b0(Cpu *c);
void f_0074d370(Cpu *c);
void f_00751930(Cpu *c);

/* -- registry: archive blocks and image states, keyed by name --------------------------------- */
typedef struct {
    char *name;
    uint32_t block;        /* ResourceBlock of "gfx/<name>" in the archive, if any */
    uint8_t eager, deferred, decoded, smooth, mask, outline;
} Entry;
enum { TABLE_SIZE = 16384 };
static Entry table[TABLE_SIZE];
static pthread_mutex_t table_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t decode_lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned deferred_count, decoded_on_demand;

static unsigned hash(const char *s) {
    unsigned h = 2166136261u;
    while (*s) h = (h ^ (uint8_t)*s++) * 16777619u;
    return h;
}
/* Entry for name, created when missing (caller holds table_lock). */
static Entry *entry_for(const char *name) {
    for (unsigned i = hash(name) & (TABLE_SIZE - 1);; i = (i + 1) & (TABLE_SIZE - 1)) {
        if (!table[i].name) { table[i].name = strdup(name); return &table[i]; }
        if (!strcmp(table[i].name, name)) return &table[i];
    }
}

static void read_string(uint32_t s, char *out, size_t size) {
    uint32_t len = rd32(s + 16), cap = rd32(s + 20);
    uint32_t data = cap >= 16 ? rd32(s) : s;
    if (len >= size) len = (uint32_t)size - 1;
    memcpy(out, G(data), len);
    out[len] = 0;
}

/* archive.c: every record, so a deferred image can pin its compressed PNG. */
void lazy_register_block(const char *name, uint32_t block) {
    if (strncmp(name, "gfx/", 4)) return;
    pthread_mutex_lock(&table_lock);
    entry_for(name + 4)->block = block;
    pthread_mutex_unlock(&table_lock);
}

static __thread int decoding; /* set while the original loadImage runs on demand */

/* -- hooks ------------------------------------------------------------------------------------- */
static void queue_task(Cpu *c) {
    uint32_t type = c->ecx.d;
    /* PROCESS_IMAGE 5, IMAGE_REPEAT 7, PROCESS_VFONT 12 / _MINUS 13 read the decoded image */
    if (type == 5 || type == 7 || type == 12 || type == 13) {
        char name[1024];
        read_string(c->esp.d + 4, name, sizeof name);
        pthread_mutex_lock(&table_lock);
        entry_for(name)->eager = 1;
        pthread_mutex_unlock(&table_lock);
    }
    f_0074fe60(c);
}

static void load_image(Cpu *c) {
    char name[1024];
    uint32_t path = c->esp.d + 8;
    read_string(path, name, sizeof name);
    pthread_mutex_lock(&table_lock);
    Entry *e = entry_for(name);
    int defer = !decoding && !e->eager && e->block;
    if (defer) {
        e->smooth |= (uint8_t)(c->ecx.b.l != 0);
        e->mask |= (uint8_t)(c->edx.b.l != 0);
        e->outline |= (uint8_t)(rd8(c->esp.d + 4) != 0);
        if (!e->deferred) deferred_count++;
        e->deferred = 1;
        wr8(e->block + 8, 1); /* managed: kept past the end of loading, until decoded */
    }
    pthread_mutex_unlock(&table_lock);
    if (!defer) {
        f_0074ab70(c);
        return;
    }
    if (rd32(path + 20) >= 16) heap_free(rd32(path)); /* the callee destroys its by-value string */
    c->eax.d = 1;
    c->esp.d += 4; /* cdecl: the caller pops the arguments */
}

/* Decodes a deferred image now, with the original loadImage and the recorded flags. */
static void prepare(Cpu *c, const char *name) {
    pthread_mutex_lock(&table_lock);
    Entry *e = entry_for(name);
    int needed = e->deferred && !e->decoded;
    uint8_t smooth = e->smooth, mask = e->mask, outline = e->outline;
    pthread_mutex_unlock(&table_lock);
    if (!needed) return;
    pthread_mutex_lock(&decode_lock);
    pthread_mutex_lock(&table_lock);
    needed = !e->decoded;
    pthread_mutex_unlock(&table_lock);
    if (needed) {
        uint32_t saved[3] = {c->eax.d, c->ecx.d, c->edx.d};
        uint32_t len = (uint32_t)strlen(name);
        c->esp.d -= 24; /* std::string argument, built in place */
        uint32_t s = c->esp.d;
        memset(G(s), 0, 24);
        if (len < 16) {
            memcpy(G(s), name, len + 1);
            wr32(s + 20, 15);
        } else {
            uint32_t p = heap_alloc(len + 1, 0);
            memcpy(G(p), name, len + 1);
            wr32(s, p);
            wr32(s + 20, len);
        }
        wr32(s + 16, len);
        push32(c, outline);
        push32(c, GUEST_RETURN_SENTINEL);
        c->ecx.b.l = smooth;
        c->edx.b.l = mask;
        decoding = 1;
        guest_call(c, LOAD_IMAGE);
        decoding = 0;
        c->esp.d += 28;
        c->eax.d = saved[0]; c->ecx.d = saved[1]; c->edx.d = saved[2];
        pthread_mutex_lock(&table_lock);
        e->decoded = 1;
        decoded_on_demand++;
        if (e->block && rd32(e->block)) { /* the compressed PNG is not needed once decoded */
            heap_free(rd32(e->block));
            wr32(e->block, 0);
            wr32(e->block + 4, 0);
        }
        pthread_mutex_unlock(&table_lock);
    }
    pthread_mutex_unlock(&decode_lock);
}

static void prepare_arg(Cpu *c, uint32_t string) {
    char name[1024];
    read_string(string, name, sizeof name);
    prepare(c, name);
}
static void get_texture(Cpu *c) { prepare_arg(c, c->esp.d + 4); f_0074cb20(c); }
static void get_texture_mask(Cpu *c) { prepare_arg(c, c->esp.d + 4); f_0074cdf0(c); }
static void get_texture_outline(Cpu *c) { prepare_arg(c, c->esp.d + 4); f_0074d0b0(c); }
static void get_texture_palette(Cpu *c) { prepare_arg(c, c->ecx.d); f_0074d370(c); }
static void process_palette_swap(Cpu *c) { prepare_arg(c, c->ecx.d); f_00751930(c); }

void lazy_report(void) {
    trace("# lazy images: %u deferred, %u decoded on demand, heap live %u KiB", deferred_count,
          decoded_on_demand, heap_used() / 1024);
}

void lazy_init(void) {
    if (getenv("RECOMP_EAGER_IMAGES")) return; /* the original behaviour, for comparison */
    guest_intercept(QUEUE_TASK, queue_task);
    guest_intercept(LOAD_IMAGE, load_image);
    guest_intercept(GET_TEXTURE, get_texture);
    guest_intercept(GET_TEXTURE_MASK, get_texture_mask);
    guest_intercept(GET_TEXTURE_OUTLINE, get_texture_outline);
    guest_intercept(GET_TEXTURE_PALETTE, get_texture_palette);
    guest_intercept(PROCESS_PALETTE_SWAP, process_palette_swap);
}
