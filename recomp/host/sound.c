/* sf::SoundBuffer keeps its decoded samples (std::vector<Int16> at +4) after handing them to
 * OpenAL, which holds its own copy: ~40 MiB of guest memory for Vagante's sound effects.
 * SoundBuffer::initialize (0x839b50, thiscall, ret 4) is the only caller of update (the
 * alBufferData upload; no copy constructor re-uploads), so once it succeeds the vector is
 * emptied. The duration is already stored; getSamples/getSampleCount are not used by the game.
 * MSVC allocations of 4 KiB or more are 32-byte aligned with the raw pointer at begin - 4. */
#include "win32.h"

#include <stdlib.h>

enum { SOUND_BUFFER_INITIALIZE = 0x00839b50u };
void f_00839b50(Cpu *c);
static unsigned long long released;
static unsigned buffers;

static void sound_buffer_initialize(Cpu *c) {
    uint32_t self = c->ecx.d;
    f_00839b50(c);
    if (!c->eax.b.l) return;
    uint32_t begin = rd32(self + 4), cap = rd32(self + 12);
    if (!begin || cap <= begin) return;
    uint32_t bytes = cap - begin;
    heap_free(bytes >= 4096 ? rd32(begin - 4) : begin);
    wr32(self + 4, 0); wr32(self + 8, 0); wr32(self + 12, 0);
    released += bytes;
    if (++buffers % 50 == 0)
        trace("# SoundBuffer: %u sample copies released (%llu KiB)", buffers, released / 1024);
}

__attribute__((constructor)) static void sound_init(void) {
    if (getenv("RECOMP_KEEP_SAMPLES")) return;
    guest_intercept(SOUND_BUFFER_INITIALIZE, sound_buffer_initialize);
}
