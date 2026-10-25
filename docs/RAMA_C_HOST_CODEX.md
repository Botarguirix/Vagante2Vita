# Rama C: APIs del host

Trabajo local en `rama-c`; no se modifica rama A ni Touhou. El host y
`recomp/build_host.py` estaban sin seguimiento al comenzar. Se incorporan sólo
los archivos correspondientes a estas tareas; los cambios previos del lifter y
runtime permanecen fuera de estos commits.

## Tarea 1: stubs sin servicios externos

`recomp/host/stubs.c`: cubre los 80 imports de Steam (15), Winsock (31), LDAP
(16), ADVAPI32 (11), CRYPT32/Normaliz/DirectInput/XInput (uno por DLL) y VERSION (3).
Steam devuelve cero y no despacha callbacks; LDAP no abre conexiones. Los 16
ordinales LDAP se registran como `WLDAP32.dll#N`, exactamente como los solicita
`imports_init`, además de sus nombres. Steam y LDAP usan `ret_cdecl`; las otras
DLL usan `ret_stdcall` con el número de palabras x86 de cada firma.

Fuentes de ABI consultadas:
- [Steamworks API](https://partner.steamgames.com/doc/api/steam_api).
- [Valve steamtypes.h: S_CALLTYPE = __cdecl](https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/public/steam/steamtypes.h).
- [Valve declaraciones internas y callbacks](https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/public/steam/steam_api_internal.h).
- [Wine: ordinales y ABI de WLDAP32](https://github.com/wine-mirror/wine/blob/master/dlls/wldap32/wldap32.spec).

`WSAStartup` inicializa los 400 bytes de WSADATA x86; socket y operaciones de red
fallan, las conversiones de byte order son reales y los errores Winsock son por
hilo. Los punteros de resultados no disponibles se limpian. CryptAcquireContextA
entrega el token 1, CryptGenRandom usa rand (sólo compatibilidad, no criptografía),
hash/cifrado fallan y GetUserNameA devuelve Player respetando tamaño y terminador.
DirectInput devuelve 0x80004001 y XInput 1167. VERSION informa sin versión.

Verificación ejecutada:
- `python -I recomp/build_host.py`: terminó con `built .../host/vagante_host.exe`.
- Compilar `tests/host/stubs_test.c` junto a `recomp/host/stubs.c` con GCC UCRT64,
  includes `recomp/runtime` y `recomp/host`; ejecutar `artifacts/stubs_test.exe`:
  `stubs: ABI and guest outputs OK`.
- La prueba comprueba limpieza de pila, callback con argumento uint64, ordinales,
  límites de WSADATA, errores de red, GetUserNameA con buffer pequeño, proveedor,
  bytes aleatorios y límites, DirectInput/XInput y VERSION.

Esto verifica el puente de APIs y el enlace del host; no acredita gameplay ni Vita.
