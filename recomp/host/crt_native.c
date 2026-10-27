/* Native replacements for functions of vagante.exe. An intercept takes priority over the
 * lifted function at the same address (natives.txt lists them for the lifter):
 *   archive.c  ResourceArchive::load (streaming data.vra), getMemoryBlock (frees used blocks)
 *   lazy.c     on-demand image decoding (loadImage, getTexture*, queueTask, processPaletteSwap) */
#include "win32.h"

void archive_init(void);
void lazy_init(void);

void crt_native_init(void) {
    archive_init();
    lazy_init();
}