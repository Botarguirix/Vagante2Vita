/* Code pages for MultiByteToWideChar / WideCharToMultiByte. Vagante runs on an en-US
 * Windows: CP_ACP is Windows-1252 and SFML converts its UTF-8 file names with CP_UTF8.
 * len < 0 means "up to and including the terminator", as in Win32; a NULL destination
 * asks for the required size. Returns 0 when the destination is too small. */
#include "win32.h"

enum { CP_UTF8 = 65001 };

/* Windows-1252 0x80-0x9F; the rest of the page is Latin-1. */
static const uint16_t cp1252_high[32] = {
    0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160, 0x2039,
    0x0152, 0x008D, 0x017D, 0x008F, 0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178,
};

static int put16(uint16_t *dst, int cap, int n, uint16_t v) {
    if (dst) {
        if (n >= cap) return -1;
        dst[n] = v;
    }
    return n + 1;
}

int mb_to_utf16(uint32_t cp, const uint8_t *src, int len, uint16_t *dst, int cap) {
    if (len < 0) len = (int)strlen((const char *)src) + 1;
    int n = 0;
    for (int i = 0; i < len;) {
        uint32_t ch = src[i++];
        if (cp == CP_UTF8 && ch >= 0x80) {
            int extra = ch >= 0xF0 ? 3 : ch >= 0xE0 ? 2 : ch >= 0xC0 ? 1 : 0;
            ch &= extra == 3 ? 0x07 : extra == 2 ? 0x0F : 0x1F;
            if (!extra) ch = 0xFFFD;
            for (; extra && i < len && (src[i] & 0xC0) == 0x80; extra--) ch = ch << 6 | (src[i++] & 0x3F);
            if (extra) ch = 0xFFFD;
        } else if (cp != CP_UTF8 && ch >= 0x80 && ch < 0xA0) {
            ch = cp1252_high[ch - 0x80];
        }
        if (ch >= 0x10000) {
            ch -= 0x10000;
            if ((n = put16(dst, cap, n, (uint16_t)(0xD800 | ch >> 10))) < 0) return 0;
            ch = 0xDC00 | (ch & 0x3FF);
        }
        if ((n = put16(dst, cap, n, (uint16_t)ch)) < 0) return 0;
    }
    return n;
}

static int put8(uint8_t *dst, int cap, int n, uint8_t v) {
    if (dst) {
        if (n >= cap) return -1;
        dst[n] = v;
    }
    return n + 1;
}

int utf16_to_mb(uint32_t cp, const uint16_t *src, int len, uint8_t *dst, int cap, int *defaulted) {
    if (len < 0) {
        for (len = 0; src[len]; len++) {}
        len++;
    }
    int n = 0;
    for (int i = 0; i < len; i++) {
        uint32_t ch = src[i];
        if (ch >= 0xD800 && ch < 0xDC00 && i + 1 < len && src[i + 1] >= 0xDC00 && src[i + 1] < 0xE000)
            ch = 0x10000 + ((ch - 0xD800) << 10) + (src[++i] - 0xDC00);
        if (cp == CP_UTF8) {
            if (ch < 0x80) {
                if ((n = put8(dst, cap, n, (uint8_t)ch)) < 0) return 0;
            } else {
                int extra = ch < 0x800 ? 1 : ch < 0x10000 ? 2 : 3;
                uint8_t lead = extra == 1 ? 0xC0 : extra == 2 ? 0xE0 : 0xF0;
                if ((n = put8(dst, cap, n, (uint8_t)(lead | ch >> (6 * extra)))) < 0) return 0;
                for (int k = extra - 1; k >= 0; k--)
                    if ((n = put8(dst, cap, n, (uint8_t)(0x80 | (ch >> (6 * k) & 0x3F)))) < 0) return 0;
            }
            continue;
        }
        uint8_t out = '?';
        if (ch < 0x80 || (ch >= 0xA0 && ch < 0x100)) {
            out = (uint8_t)ch;
        } else {
            int found = 0;
            for (int k = 0; k < 32; k++)
                if (cp1252_high[k] == ch) { out = (uint8_t)(0x80 + k); found = 1; break; }
            if (!found && defaulted) *defaulted = 1;
        }
        if ((n = put8(dst, cap, n, out)) < 0) return 0;
    }
    return n;
}
