#include "win32.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
uint8_t *guest_base;
static uint32_t call(const char *name) {
    const ApiEntry *e = win32_openal;
    while (e->name && strcmp(e->name, name)) ++e;
    assert(e->name); Cpu c = {0}; c.esp.d = 0x100;
    e->fn(&c); assert(c.esp.d == 0x104); return c.eax.d;
}
#define A(n, v) wr32(0x104 + (n)*4, v)
static void af(unsigned n, float v) { memcpy(G(0x104+n*4), &v, 4); }
int main(void) {
    guest_base = calloc(1, 0x8000); assert(guest_base);
    A(0, 0); uint32_t dev = call("alcOpenDevice"); assert(dev > 0x50000 && dev < 0x51000);
    A(0, dev); A(1, 0); uint32_t ctx = call("alcCreateContext"); assert(ctx > dev);
    A(0, ctx); assert(call("alcMakeContextCurrent") == ALC_TRUE);
    A(0, 1); A(1, 0x1000); call("alGenBuffers"); ALuint buffer = rd32(0x1000); assert(alIsBuffer(buffer));
    A(0, 1); A(1, 0x1100); call("alGenSources"); ALuint source = rd32(0x1100); assert(alIsSource(source));
    A(0, buffer); A(1, AL_FORMAT_MONO16); A(2, 0x2000); A(3, 800); A(4, 8000); call("alBufferData");
    A(0, buffer); A(1, AL_SIZE); A(2, 0x1200); call("alGetBufferi"); assert(rd32(0x1200) == 800);
    A(0, source); A(1, AL_GAIN); af(2, 0.375f); call("alSourcef");
    A(2, 0x1300); call("alGetSourcef"); assert(rdf32(0x1300) == 0.375f);
    A(0, source); A(1, AL_POSITION); af(2, 1.25f); af(3, -2.5f); af(4, 3.75f); call("alSource3f");
    float x, y, z; alGetSource3f(source, AL_POSITION, &x, &y, &z); assert(x == 1.25f && y == -2.5f && z == 3.75f);
    A(0, source); A(1, 1); A(2, 0x1000); call("alSourceQueueBuffers");
    A(0, source); call("alSourcePlay"); call("alSourceStop");
    A(0, source); A(1, 1); A(2, 0x1400); call("alSourceUnqueueBuffers"); assert(rd32(0x1400) == buffer);
    A(0, 1); A(1, 0x1100); call("alDeleteSources"); assert(!alIsSource(source));
    A(0, 1); A(1, 0x1000); call("alDeleteBuffers"); assert(!alIsBuffer(buffer));
    assert(alGetError() == AL_NO_ERROR);
    A(0, 0); assert(call("alcMakeContextCurrent"));
    A(0, dev); assert(!call("alcMakeContextCurrent")); /* wrong kind */
    A(0, ctx); call("alcDestroyContext"); assert(!call("alcMakeContextCurrent"));
    call("alcDestroyContext"); /* stale token is harmless */
    A(0, dev); A(1, 0); uint32_t next = call("alcCreateContext"); assert(next > ctx);
    A(0, next); call("alcDestroyContext"); A(0, dev); assert(call("alcCloseDevice"));
    assert(!call("alcCloseDevice")); A(0, 0xdeadbeef); assert(!call("alcCloseDevice"));
    free(guest_base); puts("openal: null backend, guest buffers, floats, cdecl and handles OK");
}
