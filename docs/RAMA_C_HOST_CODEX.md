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


## Tarea 10: fflush del log (2026-10-26)

Se añade recomp/host/fileio.c y se registra win32_fileio antes de las tablas
originales. Delega CreateFileA/W, WriteFile y CloseHandle en sus implementaciones
existentes. No se editan kernel32, threads, window, archive, lift ni runtime.
Registra por handle y FILE el bit GENERIC_WRITE que selecciona r+b/w+b en las
aperturas originales; cada WriteFile de esos streams termina con fflush(FILE).
Los lectores conservan su caché/buffer, no se llama fflush(NULL) y los handles
estándar no se vacían. El modo se limpia antes de CloseHandle y se registra de
nuevo al reutilizar un slot. El mutex sólo protege el modo, no fwrite/fflush.
Un error de fflush devuelve FALSE y ERROR_WRITE_FAULT (29), conservando el
número de bytes que aceptó fwrite. Se preserva la limpieza de pila stdcall.
fflush vacía stdio al sistema operativo; no es fsync ni garantía ante corte de
energía. El objetivo es conservar la cola al matar/fallar el proceso.

### Pruebas funcionales

`python -I tests/host/run_fileio_tests.py` pasó: aperturas A/W, escritura visible
sin CloseHandle, escritura de cero bytes, lectores reutilizados sin flush,
reutilización de handles, stdout/handle inválido sin fflush(NULL), error de
vaciado y ABI. El spy distingue el fflush(NULL) del CRT al salir, que no es una
llamada del adaptador. Dos procesos hijos con stdio totalmente buffered escriben
la misma línea y esperan; el padre termina únicamente esos hijos por PID sin
fclose. El control pierde la cola y el adaptador conserva la línea completa.
`python -I tests/host/run_heap_tests.py --stress-only` también pasó tras añadir
la tabla vacía fileio a su fixture: 72.000 operaciones, sin corrupción/solapamiento
y memoria viva final cero.

### Carga real en PC y límite del 2 %

Se construyeron dos ejecutables aislados con los mismos objetos x86→C ya
compilados, runtime, archive y librerías. Sólo se reemplaza el lookup de support.o
y se añade fileio.o en el candidato. Ambos incorporan la misma sonda exclusiva
de pruebas: intercepta el final real de ResourceManager::finishInitialization
(0x0073c6c0), llama al original y mide platform_ticks_us desde el inicio del host.
Después termina su propio proceso mediante TerminateProcess, sin limpieza del
CRT. La sonda no se incorpora al host de producción ni al VPK.

Datos del usuario reales, todas las paletas, backend OpenAL null, misma copia
congelada de config/custom y perfiles de prueba nuevos. data.vra se enlaza por
hardlink a cada perfil. Una carga de calentamiento por ejecutable, excluida;
siete pares medidos, orden alternado A/B y B/A. No se cerró la instancia del
usuario que tenía vagante_host.exe abierto: se enlazó en otra carpeta porque
Windows impedía reemplazar ese ejecutable. No se usó una prueba sintética de
WriteFile para sustituir la carga del juego.

| Mediana de 7 cargas | Segundos |
|---|---:|
| Antes | 8.566249 |
| Con fflush | 7.834515 |

Cambio observado de medianas: **-8.542 %**, dentro del límite
solicitado de +2 %. Hay dispersión entre corridas; no se atribuye la aparente
aceleración al fflush ni se garantiza ese porcentaje en cualquier equipo/carga.
La compuerta automatizada rechaza resultados con aumento de mediana >2 %.

Muestras antes (s): [8.768762, 8.268272, 10.34156, 7.765674, 8.141759, 8.608591, 8.566249].
Muestras después (s): [8.416425, 9.831034, 7.834515, 7.099964, 8.527467, 7.755557, 7.331333].
En los siete controles el log al terminar sin cerrar mide 4096 B; en los siete
candidatos conserva 4400 B, incluidos los 304 B de cola que faltaban.

SHA-256 del ejecutable de referencia con sonda:
`a0fc035e466d1081928aa04bd4a82a7e87e0825703abdb9f7ed49cc192357845`.
SHA-256 del candidato con sonda:
`5b7c3365d6382be5ed01748cf90c019a4bdbec736dcfcb1b316269f57eb37adf`.
SHA-256 de fileio.c medido:
`8da74b6c1a01fa9472670bf0eb2a82365b987dedab81c3011315135a8f65458b`.

Para repetir con los datos privados instalados y las unidades generadas:

```powershell
python -I tests/host/log_flush_load_bench.py --freeze-baseline
python -I tests/host/log_flush_load_bench.py --candidate --run --pairs 7
python -I tests/host/log_flush_load_bench.py --check-result
```

La referencia excluye fileio.o y reconstruye el lookup anterior incluso después
de instalar este cambio. Los objetos/headers congelados y los perfiles se guardan
en artifacts/fileio-load; resultados completos en results.json. Logs: artifacts/
fileio-tests-final.log, fileio-load-results.log, fileio-load-gate.log y
fileio-heap-regression.log. Los dos ejecutables PC enlazaron y completaron la
carga. La compilación ARM de la capa host también terminó con
"OK: host ARM compiled (no ELF/VPK requested)", mediante build_vita.py --host-only.
No se generó un VPK nuevo en esta tarea ni se ejecutó/midió en Vita física.
