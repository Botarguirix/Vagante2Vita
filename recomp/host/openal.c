/* OpenAL Soft bridge. All guest exports are cdecl, including alc*. */
#include "win32.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <pthread.h>
#define API(name) static void api_##name(Cpu *c)
#define I(n) ((ALint)ARG(c, n))
#define F(n) rdf32(c->esp.d + 4u + 4u * (n))
#define P(n) (ARG(c, n) ? (void *)G(ARG(c, n)) : NULL)
#define DONE() ret_cdecl(c, 0)
_Static_assert(sizeof(ALuint) == 4 && sizeof(ALint) == 4 && sizeof(ALfloat) == 4, "OpenAL guest ABI");
_Static_assert(sizeof(ALCint) == 4, "ALC attributes are guest int32");

/* Never reuse an issued token: a stale guest handle must not refer to a new
   context. Allocation failure closes/destroys the newly created host object. */
enum { ALC_TOKEN_BASE = 0x00050000u, ALC_SLOTS = 4096, DEVICE = 1, CONTEXT = 2 };
static struct { void *ptr; int kind; } alc_handles[ALC_SLOTS];
static unsigned alc_next = 1;
static pthread_mutex_t alc_lock = PTHREAD_MUTEX_INITIALIZER;
static uint32_t keep(void *ptr, int kind) {
    if (!ptr) return 0;
    pthread_mutex_lock(&alc_lock);
    unsigned i = alc_next < ALC_SLOTS ? alc_next++ : 0;
    if (i) { alc_handles[i].ptr = ptr; alc_handles[i].kind = kind; }
    pthread_mutex_unlock(&alc_lock);
    return i ? ALC_TOKEN_BASE + i : 0;
}
static void *get(uint32_t token, int kind) {
    unsigned i = token - ALC_TOKEN_BASE;
    pthread_mutex_lock(&alc_lock);
    void *ptr = i > 0 && i < alc_next && alc_handles[i].kind == kind ? alc_handles[i].ptr : NULL;
    pthread_mutex_unlock(&alc_lock);
    return ptr;
}
static void forget(uint32_t token) {
    pthread_mutex_lock(&alc_lock);
    alc_handles[token - ALC_TOKEN_BASE].ptr = NULL;
    pthread_mutex_unlock(&alc_lock);
}
API(alcOpenDevice) {
    ALCdevice *device = alcOpenDevice(P(0));
    uint32_t token = keep(device, DEVICE);
    if (device && !token) alcCloseDevice(device);
    ret_cdecl(c, token);
}
API(alcCloseDevice) {
    uint32_t token = ARG(c, 0);
    ALCdevice *device = get(token, DEVICE);
    ALCboolean ok = device ? alcCloseDevice(device) : ALC_FALSE;
    if (ok) forget(token);
    ret_cdecl(c, ok);
}
API(alcCreateContext) {
    ALCdevice *device = get(ARG(c, 0), DEVICE);
    ALCcontext *context = device ? alcCreateContext(device, P(1)) : NULL;
    uint32_t token = keep(context, CONTEXT);
    if (context && !token) alcDestroyContext(context);
    ret_cdecl(c, token);
}
API(alcDestroyContext) {
    uint32_t token = ARG(c, 0);
    ALCcontext *context = get(token, CONTEXT);
    if (context) { alcDestroyContext(context); forget(token); }
    DONE();
}
API(alcMakeContextCurrent) {
    uint32_t token = ARG(c, 0);
    ALCcontext *context = get(token, CONTEXT);
    ret_cdecl(c, token && !context ? ALC_FALSE : alcMakeContextCurrent(context));
}
API(alBufferData) { alBufferData(ARG(c, 0), I(1), P(2), I(3), I(4)); DONE(); }
API(alDeleteBuffers) { alDeleteBuffers(I(0), P(1)); DONE(); }
API(alDeleteSources) { alDeleteSources(I(0), P(1)); DONE(); }
API(alGenBuffers) { alGenBuffers(I(0), P(1)); DONE(); }
API(alGenSources) { alGenSources(I(0), P(1)); DONE(); }
API(alGetBufferi) { alGetBufferi(ARG(c, 0), I(1), P(2)); DONE(); }
API(alGetEnumValue) { ret_cdecl(c, alGetEnumValue(P(0))); }
API(alGetSourcef) { alGetSourcef(ARG(c, 0), I(1), P(2)); DONE(); }
API(alGetSourcei) { alGetSourcei(ARG(c, 0), I(1), P(2)); DONE(); }
API(alListener3f) { alListener3f(I(0), F(1), F(2), F(3)); DONE(); }
API(alListenerf) { alListenerf(I(0), F(1)); DONE(); }
API(alListenerfv) { alListenerfv(I(0), P(1)); DONE(); }
API(alSource3f) { alSource3f(ARG(c, 0), I(1), F(2), F(3), F(4)); DONE(); }
API(alSourcePause) { alSourcePause(ARG(c, 0)); DONE(); }
API(alSourcePausev) { alSourcePausev(I(0), P(1)); DONE(); }
API(alSourcePlay) { alSourcePlay(ARG(c, 0)); DONE(); }
API(alSourcePlayv) { alSourcePlayv(I(0), P(1)); DONE(); }
API(alSourceQueueBuffers) { alSourceQueueBuffers(ARG(c, 0), I(1), P(2)); DONE(); }
API(alSourceStop) { alSourceStop(ARG(c, 0)); DONE(); }
API(alSourceStopv) { alSourceStopv(I(0), P(1)); DONE(); }
API(alSourceUnqueueBuffers) { alSourceUnqueueBuffers(ARG(c, 0), I(1), P(2)); DONE(); }
API(alSourcef) { alSourcef(ARG(c, 0), I(1), F(2)); DONE(); }
API(alSourcei) { alSourcei(ARG(c, 0), I(1), I(2)); DONE(); }
#define E(name) {#name, api_##name}
const ApiEntry win32_openal[] = {
    E(alBufferData), E(alDeleteBuffers), E(alDeleteSources), E(alGenBuffers), E(alGenSources),
    E(alGetBufferi), E(alGetEnumValue), E(alGetSourcef), E(alGetSourcei), E(alListener3f),
    E(alListenerf), E(alListenerfv), E(alSource3f), E(alSourcePause), E(alSourcePausev),
    E(alSourcePlay), E(alSourcePlayv), E(alSourceQueueBuffers), E(alSourceStop), E(alSourceStopv),
    E(alSourceUnqueueBuffers), E(alSourcef), E(alSourcei), E(alcCloseDevice), E(alcCreateContext),
    E(alcDestroyContext), E(alcMakeContextCurrent), E(alcOpenDevice), {NULL, NULL},
};
