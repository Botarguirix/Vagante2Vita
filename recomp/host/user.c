/* USER32, GDI32, WINMM, OLE32, IMM32 and VERSION for vagante.exe. The window and its
 * OpenGL context come from SDL2 (window.c); here only what the game asks of Win32. */
#include "win32.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define API(name) static void name(Cpu *c)

/* -- WINMM -------------------------------------------------------------------------- */
API(timeBeginPeriod) { ret_stdcall(c, 0, 1); }
API(timeEndPeriod) { ret_stdcall(c, 0, 1); }
API(timeGetDevCaps) { /* TIMECAPS: wPeriodMin 1 ms, wPeriodMax */
    wr32(ARG(c, 0), 1); wr32(ARG(c, 0) + 4, 1000000);
    ret_stdcall(c, 0, 2);
}
API(timeGetTime) { ret_stdcall(c, (uint32_t)(platform_ticks_us() / 1000u), 0); }

/* -- USER32 ------------------------------------------------------------------------- */
API(MessageBoxA) {
    trace("MessageBoxA(title=\"%s\"): %s", ARG(c, 2) ? STR(ARG(c, 2)) : "",
          ARG(c, 1) ? STR(ARG(c, 1)) : "");
    ret_stdcall(c, 1 /* IDOK */, 4);
}

/* wsprintfA(buffer, format, ...): cdecl with guest varargs. Supports the conversions
   wsprintf has (%d %i %u %x %X %c %s %%, flags '-' '0' '#', width, 'l'/'h'). */
API(wsprintfA) {
    uint32_t out = ARG(c, 0);
    const char *f = STR(ARG(c, 1));
    unsigned arg = 2;
    char buf[1025];
    size_t n = 0;
    while (*f && n < 1024) {
        if (*f != '%') { buf[n++] = *f++; continue; }
        char spec[16] = "%";
        size_t k = 1;
        f++;
        while (*f && strchr("-0#", *f) && k < 6) spec[k++] = *f++;
        while (*f >= '0' && *f <= '9' && k < 10) spec[k++] = *f++;
        while (*f == 'l' || *f == 'h') f++;
        char conv = *f ? *f++ : 0;
        char piece[512];
        if (conv == 's') {
            uint32_t s = ARG(c, arg++);
            spec[k++] = 's'; spec[k] = 0;
            snprintf(piece, sizeof piece, spec, s ? STR(s) : "(null)");
        } else if (conv == 'c') {
            spec[k++] = 'c'; spec[k] = 0;
            snprintf(piece, sizeof piece, spec, (int)(uint8_t)ARG(c, arg++));
        } else if (conv == 'd' || conv == 'i') {
            spec[k++] = 'd'; spec[k] = 0;
            snprintf(piece, sizeof piece, spec, (int32_t)ARG(c, arg++));
        } else if (conv == 'u' || conv == 'x' || conv == 'X') {
            spec[k++] = conv; spec[k] = 0;
            snprintf(piece, sizeof piece, spec, ARG(c, arg++));
        } else {
            snprintf(piece, sizeof piece, "%c", conv ? conv : '%');
        }
        for (const char *p = piece; *p && n < 1024; p++) buf[n++] = *p;
    }
    buf[n] = 0;
    memcpy(G(out), buf, n + 1);
    ret_cdecl(c, (uint32_t)n);
}


/* -- IMM32 ----------------------------------------------------------------------------- */
API(ImmGetContext) { ret_stdcall(c, 0, 1); }
API(ImmSetCompositionWindow) { ret_stdcall(c, 1, 2); }

/* -- OLE32 ----------------------------------------------------------------------------- */
API(CoInitialize) { ret_stdcall(c, 0, 1); }
API(CoUninitialize) { ret_stdcall(c, 0, 0); }

#define E(name) {#name, name}
const ApiEntry win32_user[] = {
    E(timeBeginPeriod), E(timeEndPeriod), E(timeGetDevCaps), E(timeGetTime), E(MessageBoxA), E(wsprintfA),
    E(ImmGetContext), E(ImmSetCompositionWindow), E(CoInitialize), E(CoUninitialize),
    {NULL, NULL},
};