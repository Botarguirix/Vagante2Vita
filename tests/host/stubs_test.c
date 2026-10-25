#include "win32.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
uint8_t *guest_base;
uint32_t last_error;
static void call(const char *name, unsigned argc, int cdecl, uint32_t result) {
    const ApiEntry *e = win32_stubs;
    while (e->name && strcmp(e->name, name)) ++e;
    assert(e->name);
    Cpu c = {0}; c.esp.d = 0x100;
    e->fn(&c);
    assert(c.esp.d == 0x104 + (cdecl ? 0 : 4 * argc));
    assert(c.eax.d == result);
}
int main(void) {
    guest_base = calloc(1, 0x4000); assert(guest_base);
    /* Deliberately nonzero arguments: a cdecl wrapper must leave them in place. */
    wr32(0x104, 123); wr32(0x108, 456); wr32(0x10c, 789);
    call("SteamAPI_Init", 0, 1, 0);
    call("SteamAPI_RegisterCallResult", 3, 1, 0); /* uint64 call handle */
    call("SteamAPI_RegisterCallback", 2, 1, 0);
    call("SteamInternal_FindOrCreateUserInterface", 2, 1, 0);
    call("WLDAP32.dll#143", 2, 1, 0);
    call("WLDAP32.dll#60", 3, 1, 81);
    wr32(0x104, 0x202); wr32(0x108, 0x1000);
    memset(G(0x1000), 0xcc, 404);
    call("WSAStartup", 2, 0, 0);
    assert(rd16(0x1000) == 0x202 && rd16(0x1002) == 0x202);
    assert(rd32(0x118c) == 0 && rd32(0x1190) == 0xcccccccc);
    call("socket", 3, 0, UINT32_MAX);
    call("WSAGetLastError", 0, 0, 10050);
    wr32(0x110, 0x1800); wr32(0x1800, 123);
    call("getaddrinfo", 4, 0, 11001); assert(rd32(0x1800) == 0);
    wr32(0x104, 0x2000); wr32(0x108, 0x2100); wr32(0x2100, 6);
    call("GetUserNameA", 2, 0, 0); assert(last_error == 122 && rd32(0x2100) == 7);
    wr32(0x2100, 7); call("GetUserNameA", 2, 0, 1); assert(!strcmp(STR(0x2000), "Player"));
    wr32(0x104, 0x2200); call("CryptAcquireContextA", 5, 0, 1); assert(rd32(0x2200) == 1);
    wr32(0x104, 1); wr32(0x108, 16); wr32(0x10c, 0x2300);
    memset(G(0x22ff), 0xcc, 18); srand(7); unsigned char expected[16];
    for (int i = 0; i < 16; ++i) expected[i] = (unsigned char)rand();
    srand(7); call("CryptGenRandom", 3, 0, 1);
    assert(!memcmp(G(0x2300), expected, 16) && rd8(0x22ff) == 0xcc && rd8(0x2310) == 0xcc);
    call("CryptEncrypt", 7, 0, 0);
    wr32(0x110, 0x2400); wr32(0x2400, 123);
    call("DirectInput8Create", 5, 0, 0x80004001); assert(rd32(0x2400) == 0);
    call("XInputGetState", 2, 0, 1167);
    wr32(0x108, 0x2400); call("GetFileVersionInfoSizeA", 2, 0, 0); assert(!rd32(0x2400));
    free(guest_base); puts("stubs: ABI and guest outputs OK");
}
