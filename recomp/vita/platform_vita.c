/* Vita platform hooks for the recompiled host (H1): an on-screen log of the trace,
 * microsecond time, and a final report written to ux0:data/Vagante/
 * (recomp-result.txt and a BMP capture of the screen) for automated runs. */
#include "win32.h"

#include <psp2/display.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/power.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { W = 960, H = 544, LINES = 32, COLS = 104, CELL_W = 12, CELL_H = 16, ADVANCE = 9 };

/* Without this, VitaSDK's newlib reserves most of the user memory for malloc at
   start-up and the 256 MiB guest window no longer fits (seen on real hardware;
   Vita3K is more permissive). The guest block is separate from this bounded
   host heap used by SDL, audio, loading buffers and diagnostic text. */
int _newlib_heap_size_user = 32 * 1024 * 1024;

/* The recompiled x86 is CPU bound: run at the highest clocks an application may request. */
__attribute__((constructor)) static void max_clocks(void) {
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);
}

void platform_memory_report(char *out, size_t size) {
    SceKernelFreeMemorySizeInfo info = {sizeof info};
    if (sceKernelGetFreeMemorySize(&info) < 0) {
        snprintf(out, size, "free memory unknown");
        return;
    }
    snprintf(out, size, "free memory: user %u KiB, cdram %u KiB, phycont %u KiB",
             (unsigned)(info.size_user / 1024), (unsigned)(info.size_cdram / 1024),
             (unsigned)(info.size_phycont / 1024));
}

static uint32_t *frame;
static uint32_t *font;
static unsigned font_w, font_h;
static char lines[LINES][COLS + 1];
static unsigned line_count;
static uint64_t last_draw;
static int display_taken; /* vitaGL owns the screen from CreateDevice on */

void platform_display_taken(void) { display_taken = 1; }

static void init_display(void) {
    if (frame)
        return;
    SceUID block = sceKernelAllocMemBlock("Vagante frame", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
                                          0x200000, NULL);
    void *base = NULL;
    if (block < 0 || sceKernelGetMemBlockBase(block, &base) < 0)
        return;
    frame = base;
    memset(frame, 0, W * H * 4);
    SceDisplayFrameBuf fb = {sizeof fb, frame, W, SCE_DISPLAY_PIXELFORMAT_A8B8G8R8, W, H};
    sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
    FILE *f = fopen("app0:assets/font.rgba", "rb");
    if (f) {
        uint32_t size[2];
        if (fread(size, 4, 2, f) == 2 && (font = malloc((size_t)size[0] * size[1] * 4)) &&
            fread(font, 4, (size_t)size[0] * size[1], f) == (size_t)size[0] * size[1]) {
            font_w = size[0];
            font_h = size[1];
        }
        fclose(f);
    }
}

static void draw_text(int x, int y, const char *s, uint32_t color) {
    for (; *s && x + CELL_W <= W; s++, x += ADVANCE) {
        unsigned ch = (unsigned char)*s;
        if (ch < 32 || ch >= 127)
            ch = '?';
        if (!font_w)
            continue;
        for (int py = 0; py < CELL_H; py++)
            for (int px = 0; px < CELL_W; px++) {
                uint32_t p = font[(ch / 16 * 16 + py) * font_w + ch % 16 * 12 + px];
                if (p >> 24)
                    frame[(y + py) * W + x + px] = color;
            }
    }
}

static void redraw(void) {
    if (display_taken)
        return;
    init_display();
    if (!frame)
        return;
    memset(frame, 0, W * H * 4);
    draw_text(8, 4, "Vagante rama C - recompilacion estatica", 0xFF60D0FFu);
    unsigned first = line_count > LINES - 2 ? line_count - (LINES - 2) : 0;
    for (unsigned i = first; i < line_count; i++)
        draw_text(8, 4 + (int)(i - first + 1) * CELL_H, lines[i % LINES],
                  lines[i % LINES][0] == '#' ? 0xFFA0A0A0u : 0xFFFFFFFFu);
}

uint64_t platform_ticks_us(void) { return sceKernelGetProcessTimeWide(); }

void platform_sleep_ms(uint32_t ms) { sceKernelDelayThread(ms * 1000u); }

void platform_log(const char *line) {
    if (!strncmp(line, "#   ", 4) || !strncmp(line, "GetProcAddress", 14) || !strncmp(line, "GetModuleHandleA", 16))
        return; /* per-call detail stays in the trace file; the screen shows milestones */
    snprintf(lines[line_count % LINES], COLS + 1, "%s", line);
    line_count++;
    uint64_t now = platform_ticks_us();
    if (now - last_draw > 100000) {
        last_draw = now;
        redraw();
    }
}

static void write_bmp(const char *path) {
    if (!frame) return;
    FILE *f = fopen(path, "wb");
    if (!f)
        return;
    uint32_t data = W * H * 3, header[13] = {0, 0, 54, 40, W, (uint32_t)-H, 0x00180001u, 0, data, 2835, 2835, 0, 0};
    header[0] = 54 + data;
    fwrite("BM", 1, 2, f);
    fwrite(header, 4, 13, f);
    for (int i = 0; i < W * H; i++) {
        uint32_t p = frame[i];
        uint8_t bgr[3] = {(uint8_t)(p >> 16), (uint8_t)(p >> 8), (uint8_t)p};
        fwrite(bgr, 1, 3, f);
    }
    fclose(f);
}

void platform_finish(const char *summary) {
    platform_log(summary);
    redraw();
    char path[1100];
    snprintf(path, sizeof path, "%s/recomp-screen.bmp", host_data_dir);
    if (!display_taken) /* otherwise the renderer wrote the game frame there */
        write_bmp(path);
    snprintf(path, sizeof path, "%s/recomp-result.txt", host_data_dir);
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "%s\n", summary);
        fclose(f);
    }
    if (!display_taken)
        sceKernelDelayThread(5 * 1000 * 1000); /* leave the result on screen */
    sceKernelExitProcess(0);
}

/* Guest textures live in the guest window; host-side allocations use newlib. */
void *platform_texture_alloc(size_t bytes) { return calloc(1, bytes); }
void platform_texture_free(void *p) { free(p); }
void texture_pool_reserve(void) {}
