/* Native replacements for functions of vagante.exe. An intercept takes priority over the
 * lifted function at the same address:
 *   0x739180  ResourceArchive::load: streaming data.vra loader (archive.c). */
#include "win32.h"

void archive_init(void);

void crt_native_init(void) { archive_init(); }