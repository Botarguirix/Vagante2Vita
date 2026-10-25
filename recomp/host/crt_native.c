/* Native replacements for statically linked MSVC CRT routines. An intercept takes
 * priority over the lifted function at the same address. None yet for vagante.exe:
 * candidates (memcpy, memset, strlen) come from the PDB once the boot runs. */
#include "win32.h"

void crt_native_init(void) {}