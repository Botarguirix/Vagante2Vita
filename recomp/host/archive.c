/* Native ResourceArchive::load (0x739180) for vagante.exe.
 *
 * The original inflates the whole data.vra into a std::stringstream (its buffer grows by
 * doubling up to a single 128 MiB allocation) and then copies every record into its own
 * block: well over 250 MiB at the peak, more than the Vita's guest window. This version
 * inflates the file in small chunks on the host and creates each record directly in guest
 * memory, in the same structures the original builds:
 *   - every resource: new char[size] + new ResourceBlock{data, size, managed = false},
 *     stored with the game's own std::map<std::string, ResourceBlock*>::operator[] (0x73b040);
 *   - then three NUL-terminated text blobs, in room, palette, attack order.
 * Layout (PDB): ResourceArchive { bool loaded +0; memoryBlocks +8; roomData +0x1C;
 * attackData +0x20; paletteData +0x24 }, ResourceBlock { data +0; size +4; managed +8 }.
 * Guest operator new/new[] end in HeapAlloc, i.e. heap_alloc, so the game frees these
 * blocks with its own delete as usual. */
#include "win32.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

enum { ARCHIVE_LOAD = 0x00739180u, MAP_INDEX = 0x0073b040u };
void lazy_register_block(const char *name, uint32_t block); /* lazy.c */
void lazy_report(void);

typedef struct {
    FILE *f;
    z_stream z;
    int raw, done;
    unsigned char in[0x10000];
} Inflater;

/* Exactly n bytes of the inflated stream into dst (host pointer); 0 on a short read. */
static int read_exact(Inflater *s, void *dst, uint32_t n) {
    if (s->raw) return fread(dst, 1, n, s->f) == n;
    s->z.next_out = dst;
    s->z.avail_out = n;
    while (s->z.avail_out) {
        if (s->done) return 0;
        if (!s->z.avail_in) {
            s->z.avail_in = (uInt)fread(s->in, 1, sizeof s->in, s->f);
            s->z.next_in = s->in;
            if (!s->z.avail_in) return 0;
        }
        int r = inflate(&s->z, Z_NO_FLUSH);
        if (r == Z_STREAM_END) s->done = 1;
        else if (r != Z_OK && r != Z_BUF_ERROR) return 0;
    }
    return 1;
}
static int read_u32(Inflater *s, uint32_t *v) { return read_exact(s, v, 4); }

/* MSVC std::string in guest memory: 16-byte buffer (or heap pointer), size, capacity. */
static uint32_t guest_string(const char *text, uint32_t len) {
    uint32_t str = heap_alloc(24, 1);
    if (len < 16) {
        memcpy(G(str), text, len);
        wr8(str + len, 0);
        wr32(str + 20, 15);
    } else {
        uint32_t p = heap_alloc(len + 1, 0);
        memcpy(G(p), text, len);
        wr8(p + len, 0);
        wr32(str, p);
        wr32(str + 20, len);
    }
    wr32(str + 16, len);
    return str;
}
static void free_guest_string(uint32_t str) {
    if (rd32(str + 20) >= 16) heap_free(rd32(str));
    heap_free(str);
}

/* map.operator[](key) -> address of the mapped ResourceBlock* (thiscall, ret 4). */
static uint32_t map_index(Cpu *c, uint32_t map, uint32_t key) {
    uint32_t eax = c->eax.d, ecx = c->ecx.d, edx = c->edx.d;
    push32(c, key);
    push32(c, GUEST_RETURN_SENTINEL);
    c->ecx.d = map;
    guest_call(c, MAP_INDEX);
    uint32_t slot = c->eax.d;
    c->eax.d = eax; c->ecx.d = ecx; c->edx.d = edx;
    return slot;
}

static int read_name(Inflater *s, char *name, uint32_t cap, uint32_t *len) {
    if (!read_u32(s, len) || *len >= cap) return 0;
    if (!read_exact(s, name, *len)) return 0;
    name[*len] = 0;
    return 1;
}

/* -- spilled images ----------------------------------------------------------------------------
 * The compressed PNGs ("gfx/...") are not kept in guest memory: with on-demand decoding most of
 * them would wait there for the whole game (~70 MiB). They go to a cache file next to data.vra
 * (rewritten only when data.vra changes) and their ResourceBlock keeps data = NULL until
 * getMemoryBlock hands the block out; then the bytes are read back into the guest heap. */
typedef struct { uint32_t block, size; uint64_t offset; } Spill;
enum { SPILL_SLOTS = 16384, SPILL_MIN = 1024 };
static Spill spills[SPILL_SLOTS];
static FILE *spill_file;
static pthread_mutex_t spill_lock = PTHREAD_MUTEX_INITIALIZER;
static Spill *spill_slot(uint32_t block) {
    for (unsigned i = (block >> 2) * 2654435761u & (SPILL_SLOTS - 1);; i = (i + 1) & (SPILL_SLOTS - 1))
        if (!spills[i].block || spills[i].block == block) return &spills[i];
}
static unsigned spill_count;

/* Opens the cache: reused when its header matches data.vra, else recreated. *writing = 1 when
   the records must be written to it during this load. */
static FILE *spill_open(uint64_t vra_size, int *writing) {
    char path[1200];
    snprintf(path, sizeof path, "%s/recomp-gfx.cache", host_data_dir);
    uint64_t header[2] = {0x31786667706d6372ull /* "rcmpgfx1" */, vra_size}, seen[2] = {0, 0};
    FILE *f = fopen(path, "rb");
    if (f && fread(seen, 1, sizeof seen, f) == sizeof seen && !memcmp(seen, header, sizeof header)) {
        *writing = 0;
        return f;
    }
    if (f) fclose(f);
    f = fopen(path, "w+b");
    if (!f) return NULL;
    setvbuf(f, NULL, _IOFBF, 256 * 1024);
    fwrite(header, 1, sizeof header, f);
    *writing = 1;
    return f;
}

/* Reads a spilled block back into the guest heap (before getMemoryBlock returns it). */
static void spill_fill(uint32_t block) {
    if (!block || !spill_file || rd32(block)) return;
    pthread_mutex_lock(&spill_lock);
    Spill *s = spill_slot(block);
    if (s->block == block && s->size && !rd32(block)) {
        uint32_t data = heap_alloc(s->size, 0);
        if (data && fseek(spill_file, (long)s->offset, SEEK_SET) == 0 &&
            fread(G(data), 1, s->size, spill_file) == s->size) {
            wr32(block, data);
            wr32(block + 4, s->size);
        } else {
            trace("# spilled image block %08x: cannot read %u bytes back", block, s->size);
            if (data) heap_free(data);
        }
    }
    pthread_mutex_unlock(&spill_lock);
}

static void archive_load(Cpu *c) {
    uint32_t profile_saved=heap_profile_scope(ARCHIVE_LOAD);
    uint32_t self = c->ecx.d;
    char path[1200];
    snprintf(path, sizeof path, "%s/data.vra", host_data_dir);
    static Inflater s;
    memset(&s, 0, sizeof s);
    wr8(self, 0); /* loaded = false until the whole archive is in */
    s.f = fopen(path, "rb");
    if (!s.f) {
        trace("# ResourceArchive::load (native): cannot open %s", path);
        heap_profile_scope(profile_saved); ret_cdecl(c, 0);
        return;
    }
    setvbuf(s.f, NULL, _IOFBF, 256 * 1024);
    int first = fgetc(s.f);
    ungetc(first, s.f);
    s.raw = first != 0x78; /* zlib header; otherwise the records are stored as they are */
    if (!s.raw) inflateInit(&s.z);

    fseek(s.f, 0, SEEK_END);
    uint64_t vra_size = (uint64_t)ftell(s.f);
    fseek(s.f, 0, SEEK_SET);
    int writing = 0;
    if (spill_file) fclose(spill_file);
    spill_file = getenv("RECOMP_NO_SPILL") ? NULL : spill_open(vra_size, &writing);
    uint64_t spill_offset = 16, spilled = 0;
    unsigned char *spill_buf = NULL;
    uint32_t spill_cap = 0;

    uint64_t t0 = platform_ticks_us();
    uint32_t version = 0, count = 0, total = 0, ok = 1;
    uint16_t count16 = 0;
    ok = read_u32(&s, &version) && read_exact(&s, &count16, 2);
    count = count16;
    char name[4096];
    uint32_t len, size;
    for (uint32_t i = 0; ok && i < count; i++) {
        if (!read_name(&s, name, sizeof name, &len) || !read_u32(&s, &size)) { ok = 0; break; }
        uint32_t data = 0;
        int spill = spill_file && size >= SPILL_MIN && !strncmp(name, "gfx/", 4) && spill_count < SPILL_SLOTS / 2;
        if (spill) {
            if (size > spill_cap) {
                free(spill_buf);
                spill_buf = malloc(spill_cap = size);
                if (!spill_buf) { spill_cap = 0; ok = 0; break; }
            }
            if (!read_exact(&s, spill_buf, size)) { ok = 0; break; }
            if (writing && fwrite(spill_buf, 1, size, spill_file) != size) { ok = 0; break; }
        } else {
            data = size ? heap_alloc(size, 0) : heap_alloc(1, 0);
            if (!data || (size && !read_exact(&s, G(data), size))) { ok = 0; break; }
        }
        uint32_t block = heap_alloc(12, 1);
        if (spill) {
            Spill *sp = spill_slot(block);
            *sp = (Spill){block, size, spill_offset};
            spill_offset += size;
            spilled += size;
            spill_count++;
        }
        wr32(block, data);
        wr32(block + 4, size);
        wr8(block + 8, 0);
        uint32_t key = guest_string(name, len);
        uint32_t slot = map_index(c, self + 8, key);
        if (rd32(slot)) { /* the last occurrence of a path wins, as with operator[] = */
            if (rd32(rd32(slot))) heap_free(rd32(rd32(slot)));
            Spill *old = spill_slot(rd32(slot));
            if (old->block == rd32(slot)) old->size = 0; /* the address may be reused */
            heap_free(rd32(slot));
        }
        wr32(slot, block);
        lazy_register_block(name, block);
        free_guest_string(key);
        total += size;
    }
    static const uint32_t text_field[3] = {0x1C, 0x24, 0x20}; /* room, palette, attack */
    for (unsigned k = 0; ok && k < 3; k++) {
        if (!read_name(&s, name, sizeof name, &len) || !read_u32(&s, &size)) { ok = 0; break; }
        uint32_t text = heap_alloc(size + 1, 0);
        if (!text || (size && !read_exact(&s, G(text), size))) { ok = 0; break; }
        wr8(text + size, 0);
        uint32_t old = rd32(self + text_field[k]);
        if (old) heap_free(old);
        wr32(self + text_field[k], text);
    }
    if (!s.raw) inflateEnd(&s.z);
    fclose(s.f);
    free(spill_buf);
    if (spill_file && writing) fflush(spill_file);
    if (spill_file)
        trace("# gfx cache: %u images, %llu KiB outside guest memory (%s)", spill_count,
              (unsigned long long)(spilled / 1024), writing ? "written" : "reused");
    wr8(self, (uint8_t)ok);
    trace("# ResourceArchive::load (native): %s, %u records, %u KiB, %llu ms, heap live %u KiB",
          ok ? "loaded" : "truncated data.vra", count, total / 1024,
          (unsigned long long)((platform_ticks_us() - t0) / 1000u), heap_used() / 1024);
    heap_profile_scope(profile_saved); ret_cdecl(c, 0); /* thiscall without arguments: pop the return address */
}

/* ResourceArchive::getMemoryBlock (0x73aa40) wrapper. The original frees the unmanaged blocks
   (compressed images, sounds, shaders read once while loading) only when loading ends, so
   ~118 MiB of compressed data stays next to the decoded images. As in the rama A port, a block
   handed out to a thread is emptied when that thread asks for its next one (each loader
   thread decodes what it requested before requesting more). The ResourceBlock stays in the
   map with data = NULL, which the original cleanUpMemoryBlocks deletes safely. */
enum { GET_MEMORY_BLOCK = 0x0073aa40u };
void f_0073aa40(Cpu *c);
static pthread_key_t handed_key;
static pthread_once_t handed_once = PTHREAD_ONCE_INIT;
static void make_handed_key(void) { pthread_key_create(&handed_key, NULL); }
static unsigned long long blocks_released, bytes_released;

static void get_memory_block(Cpu *c) {
    pthread_once(&handed_once, make_handed_key);
    uint32_t previous = (uint32_t)(uintptr_t)pthread_getspecific(handed_key);
    if (previous && !rd8(previous + 8) && rd32(previous)) {
        bytes_released += rd32(previous + 4);
        blocks_released++;
        heap_free(rd32(previous));
        wr32(previous, 0);
        wr32(previous + 4, 0);
    }
    f_0073aa40(c); /* thiscall(name by value): the lifted original pops its own arguments */
    uint32_t block = c->eax.d;
    spill_fill(block);
    pthread_setspecific(handed_key, (void *)(uintptr_t)(block && !rd8(block + 8) ? block : 0));
    if (blocks_released && blocks_released % 500 == 0)
        trace("# getMemoryBlock: %llu blocks released after use (%llu KiB), heap live %u KiB", blocks_released,
              bytes_released / 1024, heap_used() / 1024);
}

/* Heap at the end of loading (and the lazy-image counters). */
enum { FINISH_INITIALIZATION = 0x0073c6c0u };
void f_0073c6c0(Cpu *c);
static void finish_initialization(Cpu *c) {
    trace("# finishInitialization: before, heap live %u KiB", heap_used() / 1024);
    f_0073c6c0(c);
    lazy_report();
    heap_profile_dump("finishInitialization");
}

void archive_init(void) {
    guest_intercept(FINISH_INITIALIZATION, finish_initialization);
    guest_intercept(ARCHIVE_LOAD, archive_load);
    guest_intercept(GET_MEMORY_BLOCK, get_memory_block);
}
