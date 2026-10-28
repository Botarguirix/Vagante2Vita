/* Performance diagnostics. Every build: frame statistics from SwapBuffers (frame time,
 * time inside the swap, draw calls per frame), traced every 5 s. Profile builds
 * (-DT123_PROFILE): a sampler thread also reads the last guest function entered every
 * millisecond and traces the 15 most sampled ones. "Last entered" puts time a caller
 * spends after a call returns on the callee, so read loops and their callees together. */
#include "win32.h"

#include <pthread.h>
#include <stdio.h>

void platform_sleep_ms(uint32_t ms);

static uint64_t last_swap, window_start, frame_sum, frame_max, swap_sum;
static unsigned frames, draws;

void profile_draw(void) { draws++; }

void profile_swap(uint64_t swap_start, uint64_t swap_end) {
    if (last_swap) {
        uint64_t frame = swap_end - last_swap;
        frame_sum += frame;
        if (frame > frame_max) frame_max = frame;
        swap_sum += swap_end - swap_start;
        frames++;
    } else {
        window_start = swap_end;
    }
    last_swap = swap_end;
    if (frames && swap_end - window_start >= 5000000u) {
        trace("# frames: %.1f fps, avg %.1f ms (max %.1f), in swap %.1f ms, %u draws/frame",
              frames * 1e6 / (double)(swap_end - window_start), frame_sum / 1000.0 / frames,
              frame_max / 1000.0, swap_sum / 1000.0 / frames, draws / frames);
        window_start = swap_end;
        frames = draws = 0;
        frame_sum = frame_max = swap_sum = 0;
    }
}

#ifdef T123_PROFILE
volatile uint32_t guest_prof_last;

enum { SLOTS = 16384 };
static struct { uint32_t addr, count; } hist[SLOTS];

static void *sampler(void *arg) {
    (void)arg;
    uint64_t start = platform_ticks_us();
    unsigned total = 0;
    for (;;) {
        platform_sleep_ms(1);
        uint32_t a = guest_prof_last;
        if (a) {
            unsigned h = (a * 2654435761u >> 8) & (SLOTS - 1);
            while (hist[h].addr && hist[h].addr != a) h = (h + 1) & (SLOTS - 1);
            hist[h].addr = a;
            hist[h].count++;
            total++;
        }
        if (platform_ticks_us() - start < 5000000u || !total) continue;
        char line[1024];
        int n = snprintf(line, sizeof line, "# profile (%u samples):", total);
        for (int k = 0; k < 15; k++) {
            unsigned best = SLOTS;
            for (unsigned i = 0; i < SLOTS; i++)
                if (hist[i].count && (best == SLOTS || hist[i].count > hist[best].count)) best = i;
            if (best == SLOTS) break;
            n += snprintf(line + n, sizeof line - n, " %08x %.1f%%", hist[best].addr,
                          100.0 * hist[best].count / total);
            hist[best].count = 0;
        }
        trace("%s", line);
        for (unsigned i = 0; i < SLOTS; i++) hist[i].addr = hist[i].count = 0;
        total = 0;
        start = platform_ticks_us();
    }
    return NULL;
}

__attribute__((constructor)) static void start_sampler(void) {
    pthread_attr_t attr;
    pthread_t t;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 64 << 10);
    pthread_create(&t, &attr, sampler, NULL);
    pthread_attr_destroy(&attr);
}
#endif
