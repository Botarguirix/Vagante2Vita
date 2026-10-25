/* PC platform hooks for the host (the Vita versions are in recomp/vita/). */
#include "win32.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void platform_log(const char *line) { (void)line; }

uint64_t platform_ticks_us(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    uint64_t now = (uint64_t)ts.tv_sec * 1000000u + (uint64_t)ts.tv_nsec / 1000u;
    static uint64_t start; /* relative to the first call, like the Vita's process time */
    if (!start) start = now;
    return now - start;
}

void platform_finish(const char *summary) { printf("%s\n", summary); }

void platform_sleep_ms(uint32_t ms) {
    struct timespec ts = {ms / 1000, (long)(ms % 1000) * 1000000L};
    nanosleep(&ts, NULL);
}

void platform_memory_report(char *out, size_t size) { snprintf(out, size, "(no memory report on PC)"); }

void *platform_texture_alloc(size_t bytes) { return calloc(1, bytes); }
void platform_texture_free(void *p) { free(p); }
