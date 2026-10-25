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

## Tarea 2: OpenAL Soft

`recomp/host/openal.c`: implementa los 28 imports OpenAL32 del inventario. Nueva
tabla `win32_openal`, declarada en win32.h y registrada en lookup de support.c.
Todos usan cdecl. Los floats se leen como float32 de la pila; arrays, datos PCM,
atributos de contexto y parámetros de salida se convierten con G, preservando
NULL. Los identificadores ALuint de buffers/sources pasan sin transformación.

Los ALCdevice/ALCcontext se guardan en una tabla con tipo, protegida por mutex,
y se exponen como tokens 0x50001 en adelante. No se reutilizan tokens: rechaza
handles caducados, ajenos o de otro tipo. Hay 4095 tokens por ejecución; si se
agotan, se destruye el objeto host recién creado y se devuelve NULL. Hacer
current el contexto NULL libera el contexto actual. La vida útil concurrente
de dispositivos/contextos sigue siendo responsabilidad del llamador OpenAL.

Enlace PC estático: build_host.py define AL_LIBTYPE_STATIC y agrega stdc++ y
avrt (OpenAL Soft usa C++ y el backend WASAPI requiere AvSetMmThreadCharacteristicsW).
SDL2/OpenGL y las otras bibliotecas existentes se conservan. También se rechaza
con mensaje claro una compilación sin unit_*.c: se observó lift_all.py regenerando
las unidades durante un intento, que dejó momentáneamente sólo table.c. No se
modificó ni interrumpió ese proceso. El primer intento de enlace OpenAL falló
por esa regeneración y por la configuración del enlace estático, ya corregida.

Prueba reproducible: `python -I tests/host/run_tests.py openal`.
Resultado observado: los 28 imports están registrados; backend null de OpenAL
Soft, carga/consulta de PCM, fuentes, parámetros float, cola/desencolado y borrado
funcionan; se verifica pila cdecl y rechazo de tokens inválidos/caducados.
El backend null verifica el flujo de audio, no una salida audible.

Compilación completa de esta tarea: `python -I recomp/build_host.py` terminó con
`built .../host/vagante_host.exe` (salida en artifacts/host-openal-build.log).
Se agregó el propio script a las dependencias de los objetos host: cambiar sus
flags debe recompilarlos, evitando reutilizar un openal.o construido para DLL al
enlazar la biblioteca estática. No fuerza a regenerar el código del lifter.

## Tarea 3: OpenGL

`recomp/host/gl.c`: nueva tabla win32_gl, declarada en win32.h y registrada en
lookup de support.c. Hay **32 gl*** en artifacts/imports.txt, no 33; sus 37
imports OPENGL32 se dividen en esas 32 funciones y cinco wgl*. No se implementó
ni modificó ninguna wgl*.

Cada wrapper llama a OpenGL del host y usa ret_stdcall. Parámetros float32 se
leen como tales; glOrtho recibe seis double64 en 12 palabras de la pila x86.
Los datos de texturas, índices, matrices, arrays de vértices/color/UV y buffers
de salida se traducen con G y los punteros nulos siguen siendo NULL. Los arrays
cliente apuntan a la memoria guest estable y deben conservarse vivos hasta dibujar.
Los GLuint son identificadores OpenGL, no direcciones, y pasan sin conversión.

glGetString conserva una copia guest por enum mediante guest_strdup; llamadas
posteriores devuelven la misma dirección. No se convierte ni trunca un puntero
host de 64 bits. NULL (por contexto ausente o enum inválido) no se almacena. La
caché tiene 16 entradas, suficientes para los enums de OpenGL 1.1 importado, y
su asignación está protegida por mutex.

Verificación reproducible: `python -I tests/host/run_tests.py`.
Se observó cobertura 80/80 stubs, 28/28 OpenAL y 32/32 OpenGL. Las tres pruebas
terminaron con código cero. La prueba GL crea una ventana SDL oculta y un contexto
real (OpenGL 4.6.0 NVIDIA 591.59 en este equipo); verifica pila, double/float,
caché de cadenas, viewport, enable/disable, matrices, arrays cliente, dibujo con
índices guest, upload/readback de texturas y modificación por subimagen.
No modifica el host de ventanas ni acredita ejecución del juego completo.


Compilación final: `python -I recomp/build_host.py` terminó con
`built .../host/vagante_host.exe` después de registrar win32_gl. Evidencia local:
artifacts/host-gl-build.log y artifacts/host-api-tests.log. No se ejecutó el
juego comercial ni se validó gameplay; la validación observada es del puente
host y de su enlace.

## Tarea 4: XInput por SDL_GameController

XInputGetState ahora abre bajo demanda hasta cuatro mandos SDL, conserva sus
slots mientras sigan conectados y rellena los 16 bytes del XINPUT_STATE x86 con
escrituras explícitas. Mapea cruceta, Start/Back, L3/R3, LB/RB y A/B/X/Y a los bits
estándar Xbox. Los gatillos se convierten de 0..32767 a 0..255; sticks X pasan
como int16 y Y se invierte, saturando -32768 a +32767. No añade zonas muertas.
El paquete aumenta sólo cuando cambia el estado visible, incluida la primera
muestra/reconexión. La tabla se protege con mutex. Sin dispositivo devuelve 1167;
un buffer NULL devuelve 87. No borra buffers al fallar y usa stdcall de dos args.
La inicialización de SDL sigue a cargo del host de ventanas existente.

Pruebas ejecutadas:
- `python -I tests/host/run_tests.py stubs xinput`: OK, 80/80 imports de stubs,
  XInput registrado y prueba con cuatro mandos virtuales SDL. Comprueba cada bit
  Xbox, gatillos intermedio/extremos, sticks extremos, Y saturada, paquete estable
  o actualizado, límites de escritura, slots independientes, desconexión,
  reconexión y limpieza de pila stdcall.
- `python -I recomp/build_host.py`: `built .../host/vagante_host.exe` (log local
  artifacts/host-xinput-build-retry.log). El primer enlace informó file truncated;
  repetir el enlace con los objetos ya compilados terminó correctamente.
- No se probó un mando físico ni los controles en Vita. Los mandos virtuales
  verifican el puente con SDL real sin depender del hardware conectado al equipo.

Los cambios preexistentes de SteamInternal_ContextInit y DirectInput se conservan
sin incorporarlos al commit de XInput. La prueba antigua de stubs admite ambos
backends de DirectInput y usa mocks sólo para la asignación de su objeto COM.
