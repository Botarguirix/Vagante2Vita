/* Offline imports. Guest ABIs are independent of the host compiler's ABI. */
#include "win32.h"
#include <stdlib.h>
#include <string.h>
#define API(name) static void name(Cpu *c)
#define S(name, value, argc) API(name) { ret_stdcall(c, value, argc); }
#define C(name, value) API(name) { ret_cdecl(c, value); }

/* Steamworks steamtypes.h defines S_CALLTYPE as __cdecl, including callbacks
   registration and the internal interface helpers. Never pop their arguments. */
C(SteamAPI_Init, 0)
C(SteamAPI_GetHSteamUser, 0)
C(SteamAPI_RegisterCallResult, 0)
C(SteamAPI_RegisterCallback, 0)
C(SteamAPI_RunCallbacks, 0)
C(SteamAPI_Shutdown, 0)
C(SteamAPI_UnregisterCallResult, 0)
C(SteamAPI_UnregisterCallback, 0)
C(SteamGameServer_GetHSteamUser, 0)
C(SteamGameServer_RunCallbacks, 0)
C(SteamGameServer_Shutdown, 0)
C(SteamInternal_ContextInit, 0)
C(SteamInternal_FindOrCreateGameServerInterface, 0)
C(SteamInternal_FindOrCreateUserInterface, 0)
C(SteamInternal_GameServer_Init, 0)

static _Thread_local uint32_t wsa_error;
#define NET(name, argc) API(name) { wsa_error = 10050; ret_stdcall(c, UINT32_MAX, argc); }
API(htonl) { ret_stdcall(c, __builtin_bswap32(ARG(c, 0)), 1); }
API(htons) { ret_stdcall(c, __builtin_bswap16((uint16_t)ARG(c, 0)), 1); }
API(WSAStartup) {
    uint32_t out = ARG(c, 1);
    if (!out) { ret_stdcall(c, 10014, 2); return; }
    /* WSADATA x86: WORDs at 0/2; description[257], status[129];
       WORD maxSockets/maxUdpDg at 390/392, padding and guest pointer at 396. */
    memset(G(out), 0, 400);
    wr16(out, (uint16_t)ARG(c, 0));
    wr16(out + 2, 0x0202);
    memcpy(G(out + 4), "Offline host", 13);
    memcpy(G(out + 261), "No networking", 14);
    wsa_error = 0;
    ret_stdcall(c, 0, 2);
}
S(WSACleanup, 0, 0)
API(WSAGetLastError) { ret_stdcall(c, wsa_error, 0); }
API(WSASetLastError) { wsa_error = ARG(c, 0); ret_stdcall(c, 0, 1); }
NET(socket, 3)
NET(accept, 3)
NET(bind, 3)
NET(closesocket, 1)
NET(connect, 3)
NET(getpeername, 3)
NET(getsockname, 3)
NET(getsockopt, 5)
NET(ioctlsocket, 3)
NET(listen, 2)
NET(recv, 4)
NET(recvfrom, 6)
NET(select, 5)
NET(send, 4)
NET(sendto, 6)
NET(setsockopt, 5)
S(__WSAFDIsSet, 0, 2)
API(WSAIoctl) {
    if (ARG(c, 6)) wr32(ARG(c, 6), 0);
    wsa_error = 10050;
    ret_stdcall(c, UINT32_MAX, 9);
}
S(freeaddrinfo, 0, 1)
API(getaddrinfo) {
    if (ARG(c, 3)) wr32(ARG(c, 3), 0);
    wsa_error = 11001;
    ret_stdcall(c, 11001, 4);
}
API(gethostname) {
    if (ARG(c, 0) && (int32_t)ARG(c, 1) > 0) wr8(ARG(c, 0), 0);
    wsa_error = 10050;
    ret_stdcall(c, UINT32_MAX, 2);
}
S(inet_addr, UINT32_MAX, 1)
S(inet_ntoa, 0, 1)

/* LDAP is cdecl as well. Ordinal aliases match the DLL-qualified names built
   by imports_init when ordinal_name has no mapping. No host networking occurs. */
C(ldap_init, 0)
C(ldap_memfree, 0)
C(ldap_set_option, 81)
C(ldap_err2string, 0)
C(ldap_first_entry, 0)
C(ldap_next_entry, 0)
C(ldap_get_dn, 0)
API(ldap_first_attribute) { if (ARG(c, 2)) wr32(ARG(c, 2), 0); ret_cdecl(c, 0); }
C(ldap_next_attribute, 0)
C(ldap_get_values_len, 0)
C(ldap_msgfree, 0)
C(ldap_unbind_s, 0)
API(ldap_search_s) { if (ARG(c, 6)) wr32(ARG(c, 6), 0); ret_cdecl(c, 81); }
C(ldap_simple_bind_s, 81)
C(ldap_value_free_len, 0)
C(ber_free, 0)
API(IdnToAscii) { last_error = 120; ret_stdcall(c, 0, 5); }
S(CertFreeCertificateContext, 1, 1)

API(CryptAcquireContextA) {
    if (!ARG(c, 0)) { last_error = 87; ret_stdcall(c, 0, 5); return; }
    wr32(ARG(c, 0), 1);  /* offline provider token, not a host pointer */
    ret_stdcall(c, 1, 5);
}
API(CryptGenRandom) {
    uint32_t size = ARG(c, 1), out = ARG(c, 2);
    if (size && !out) { last_error = 87; ret_stdcall(c, 0, 3); return; }
    for (uint32_t i = 0; i < size; i++) wr8(out + i, (uint8_t)rand());
    ret_stdcall(c, 1, 3);
}
S(CryptReleaseContext, 1, 2)
#define CRYPT_FAIL(name, argc) API(name) { last_error = 120; ret_stdcall(c, 0, argc); }
CRYPT_FAIL(CryptCreateHash, 5)
CRYPT_FAIL(CryptDestroyHash, 1)
CRYPT_FAIL(CryptDestroyKey, 1)
CRYPT_FAIL(CryptEncrypt, 7)
CRYPT_FAIL(CryptGetHashParam, 5)
CRYPT_FAIL(CryptHashData, 4)
CRYPT_FAIL(CryptImportKey, 6)
API(GetUserNameA) {
    uint32_t out = ARG(c, 0), count = ARG(c, 1);
    if (!count) { last_error = 87; ret_stdcall(c, 0, 2); return; }
    uint32_t capacity = rd32(count);
    wr32(count, 7); /* includes terminator */
    if (!out || capacity < 7) { last_error = 122; ret_stdcall(c, 0, 2); return; }
    memcpy(G(out), "Player", 7);
    ret_stdcall(c, 1, 2);
}
API(DirectInput8Create) {
    if (ARG(c, 3)) wr32(ARG(c, 3), 0);
    ret_stdcall(c, 0x80004001u, 5);
}
S(XInputGetState, 1167, 2)
API(GetFileVersionInfoSizeA) { if (ARG(c, 1)) wr32(ARG(c, 1), 0); ret_stdcall(c, 0, 2); }
S(GetFileVersionInfoA, 0, 4)
API(VerQueryValueA) {
    if (ARG(c, 2)) wr32(ARG(c, 2), 0);
    if (ARG(c, 3)) wr32(ARG(c, 3), 0);
    ret_stdcall(c, 0, 4);
}
#define E(name) {#name, name}
#define LDAP(ord, name) {"WLDAP32.dll#" #ord, name}, E(name)
const ApiEntry win32_stubs[] = {
    E(SteamAPI_Init), E(SteamAPI_GetHSteamUser), E(SteamAPI_RegisterCallResult), E(SteamAPI_RegisterCallback),
    E(SteamAPI_RunCallbacks), E(SteamAPI_Shutdown), E(SteamAPI_UnregisterCallResult), E(SteamAPI_UnregisterCallback),
    E(SteamGameServer_GetHSteamUser), E(SteamGameServer_RunCallbacks), E(SteamGameServer_Shutdown),
    E(SteamInternal_ContextInit), E(SteamInternal_FindOrCreateGameServerInterface),
    E(SteamInternal_FindOrCreateUserInterface), E(SteamInternal_GameServer_Init),
    E(htonl), E(htons), {"ntohl", htonl}, {"ntohs", htons}, E(WSAStartup), E(WSACleanup),
    E(WSAGetLastError), E(WSASetLastError), E(socket), E(accept), E(bind), E(closesocket), E(connect),
    E(getpeername), E(getsockname), E(getsockopt), E(ioctlsocket), E(listen), E(recv), E(recvfrom),
    E(select), E(send), E(sendto), E(setsockopt), E(__WSAFDIsSet), E(WSAIoctl), E(freeaddrinfo),
    E(getaddrinfo), E(gethostname), E(inet_addr), E(inet_ntoa),
    LDAP(143, ldap_init), LDAP(200, ldap_memfree), LDAP(211, ldap_set_option), LDAP(22, ldap_err2string),
    LDAP(26, ldap_first_entry), LDAP(27, ldap_next_entry), LDAP(30, ldap_get_dn),
    LDAP(32, ldap_first_attribute), LDAP(33, ldap_next_attribute), LDAP(35, ldap_get_values_len),
    LDAP(41, ldap_msgfree), LDAP(46, ldap_unbind_s), LDAP(50, ldap_search_s),
    LDAP(60, ldap_simple_bind_s), LDAP(79, ldap_value_free_len), LDAP(301, ber_free),
    E(IdnToAscii), E(CertFreeCertificateContext), E(CryptAcquireContextA), E(CryptGenRandom),
    E(CryptReleaseContext), E(CryptCreateHash), E(CryptDestroyHash), E(CryptDestroyKey), E(CryptEncrypt),
    E(CryptGetHashParam), E(CryptHashData), E(CryptImportKey), E(GetUserNameA),
    E(DirectInput8Create), E(XInputGetState), E(GetFileVersionInfoSizeA), E(GetFileVersionInfoA), E(VerQueryValueA),
    {NULL, NULL},
};
