# Iteración 01.63: consolidar la transición de la cueva al piso 0

La captura que motivó el reporte muestra el texto `O RESET`, y el usuario
confirmó que probaba el VPK 01.55. Esa entrega es anterior a la corrección de
01.59: el contacto con la salida del mapa inicial volvía a cargar la intro en
vez de abrir ACT_ONE. El flujo actual conserva la corrección de 01.59 y carga
`world/act-one/act-one-world.vwi` al tocar la meta del intro. Si faltan datos,
registra el fallo y muestra el bloqueo.

La traza de ruta `intro-route161/intro.vtr` confirma el trayecto desde la
aparición del mapa legado: mantener derecha durante 38 ticks alcanza la meta
en `(18,20)`, sin saltos; el jugador termina en `x=281.50`, solapado con la
meta, y `reached_exit=1`. Continuar a la derecha después de la abertura puede
llevar al muro. La entrada es por contacto, sin Círculo. El texto del mundo lo
indica. Se restauró Círculo como volver desde la pantalla de clase; la
iteración 01.62 lo había desactivado y ya no coincidía con su propia
indicación.

El VPK 01.63 se compiló con VitaSDK. `check_vpk.py` valida `TITLE_ID=VAGV00006`,
`APP_VER=01.63`, cabecera SELF y las siete entradas, sin errores ni
advertencias. SHA-256:
`D6CCE70DBD9FF38CA183023D433F7B6A77B6F18A772C9E296DB602D66D660AA9`.
Pasaron los 13 grupos C host, incluidos menú y flujo de campaña, y las 206
pruebas Python.

Vita3K sigue sin iniciar una sesión nativa en el entorno local. Se verificó
una copia aislada del perfil que conserva el binario 0.2.1.4170 conocido por
las pruebas históricas (hash `6C125434…C38E4B7`): 1,827 archivos copiados,
VPK 01.63 instalado y assets ACT_ONE validados (142 archivos). El smoke de
intro a ACT_ONE preparó 77 segmentos / 674 ticks, pero tras 60 s Vita3K no
creó `vita3k.log` ni una sesión nativa. Resultado:
`_local/vita3k-act-one-163-4170baseline/runs/iteration163/run-000001/result.json`.
El bloqueo ocurre antes de poder atribuir un fallo a la cueva, al runtime del
juego o a los assets. Con `--skip-title`, el depurador capturó la misma
violación en `ucrtbase.dll+0x25F31` en las builds 0.2.1.4170
(`Vita3K.exe+0x6835F5`), 0.2.1.4183 (`+0x683625`) y 0.2.1.4194 (`+0x683965`), con marcos
de `Qt6Core`. WSL también rechazó esta sesión con `E_ACCESSDENIED`, así que no
se pudo repetir la compilación/prueba host mediante WSL.

La prueba histórica exitosa sí es real, pero cubre únicamente el VPK 00.25:
15 comprobaciones nativas pasaron con Vita3K 0.2.1.4170 desde el perfil
`_local/vita3k/filesystem`. El perfil y los binarios conservados después se
modificaron/reemplazaron, y no se ha reproducido ese mismo punto de control con
01.63; tampoco valida la transición al primer mundo. Por tanto, aún falta una
base reproducible que pueda arrancar antes de usarla como regresión de la
campaña.

Se repitió el control 00.25 con la misma replay histórica: la entrada generada
coincide en SHA-256 con `_local/vita3k/runs/iteration025/run-000006/input.vts`
(`4228BEE6…F199B33`). En un clon 0.2.1.4170 de 1,827 archivos, Vita3K acabó
con `0xC0000005` antes del log nativo. Un segundo lanzamiento del mismo binario
con `--skip-title` falló en el mismo `ucrtbase.dll+0x25F31` /
`Vita3K.exe+0x6835F5`, así que el crash ocurre incluso sin intentar cargar un
título. El preflight también rechazó el manifiesto de recursos de 00.25 por
ser de formato antiguo; se aisló el contenido del intro para completar el
intento y evitar confundir ese rechazo con el fallo del emulador. Evidencia:
`_local/vita3k-control025-4170-retry/runs/iteration025/run-000001/result.json`.

Se repitió desde la instalación solicitada `C:\Vita3k` (Vita3K 0.2.1.4194,
SHA-256 `DFC5C7CB…B30C54FE`), copiando runtime y firmware a un perfil local
aislado, instalando 01.63 y entregando los mismos 142 assets validados. La
prueba volvió a acabar antes de la sesión; `--skip-title` reprodujo la misma
violación de acceso en `ucrtbase.dll+0x25F31`, ahora desde
`Vita3K.exe+0x683965`. Esto descarta que el resultado dependa de los recursos,
del VPK, del título o del renderer Vulkan. Evidencia:
`_local/vita3k-c164-4194-isolated/runs/iteration163/run-000001/result.json`.

El harness ahora limpia `ux0:data/Vagante2Vita/` y la instalación de
`ux0:app/VAGV00006` dentro del perfil de prueba antes de cada corrida, para que
no herede archivos de paquetes anteriores. También escribe `resources.json`
con hashes por archivo y registra en `run.json` las huellas de recursos, replay,
VPK y versión/hash de Vita3K. La corrida corta de verificación desde
`C:\Vita3k` registró esos datos, pero volvió a fallar antes de la sesión:
`_local/vita3k-c166-4194-harness/runs/iteration163/run-000001/`.

## Seguimiento de Worm y replay de campaña

Para habilitar campaña automatizada, el límite de replay VTS1 subió de 1,024
a 2,048 segmentos. Una prueba de regresión usa 1,505 segmentos para demostrar
que el formato admite campañas largas. Las tres rutas físicas se regeneraron
con la huella actual del simulador: Dragon y Goblin King fueron derrotados con
el jugador vivo en los pisos de prueba; el replay combinado produce 1,178
segmentos y 8,979 ticks nominales.

El nuevo Worm provisional usa 20 segmentos dibujados desde las atlas privadas
del usuario y una ruta de seguimiento; su ataque ácido y paridad nativa siguen
pendientes. El historial ya no se copia dentro de `World`, porque los probes
copian esa estructura por valor. El empaquetador Windows aún aborta con
`0xC0000374` al compilar `-O3`; con `-Optimization Os` sí se generó y validó el
VPK de prueba [Vagante2Vita-iteration163.vpk](../_local/build-act-one-replay2048-os163/Vagante2Vita-iteration163.vpk)
(SHA-256 `09EB1791BDFBE8E4B3E7A8771525462E2967582AF1975F84797CE7E64E4CA2AB`).

Pasaron 207 pruebas Python y 13 grupos C host. `check_vpk.py` valida 7 entradas
sin errores. El replay y el paquete de 142 recursos pasan el preflight de
ACT_ONE. Vita3K no creó ventana, `vita3k.log` ni sesión nativa al probar tanto
el perfil aislado limpio como la copia completa hash-verificada del perfil
configurado fuera de `C:\Vita3k`; el resultado termina en
`before_native_session`. Por tanto, esta build no está validada en emulador y
el primer mundo aún no puede considerarse jugable de extremo a extremo.

Para pruebas consistentes hay que congelar y registrar juntos: (1) una versión
de Vita3K y su SHA-256 que arranque desde este usuario/perfil; (2) un perfil
aislado con firmware, módulos, configuración y rutas válidas; (3) un estado de
título limpio para `VAGV00006`; (4) hashes de VPK y paquete de datos; y (5) una
replay cuyo resultado capture el log de sesión y el cierre del emulador. El
script actual verifica copia, instalación, identidad y paquete, pero un timeout
antes del log nativo es un fallo de infraestructura, no un smoke del juego.
El siguiente paso útil es recuperar la instalación/configuración exacta de
Windows que produjo el PASS histórico, porque los binarios 0.2.1.4170 y
0.2.1.4194 disponibles ya fallan al iniciar el frontend aun con `--skip-title`.
Después se debe repetir 00.25 como control positivo y comparar 01.63 en otra
copia limpia.
WSL permanece bloqueado con `E_ACCESSDENIED`; VitaSDK/clang de Windows sí
permite compilar.

La transición y el piso 0 aún requieren confirmación en runtime. Los datos de
ACT_ONE deben existir bajo `ux0:data/Vagante2Vita/world/act-one/`.

VPK: `_local/build-act-one-163/Vagante2Vita-iteration163.vpk` (843,360 bytes).

## Diagnóstico posterior del Dragon

El probe host tiene un ciclo parcial de acciones identificado en Ghidra
(IDLE/MOVE/FACE/SHOOT_ONCE), con persecución, proyectil y selección todavía
provisionales. En el fixture separado, el ancla `[20,29]` permite derrotar al
Dragon tras siete impactos conservando 30 HP. Un stage de navegación añadido
después de la pelea busca una columna libre para sortear el bloque sólido bajo
la ruta: el jugador alcanza la salida en 2,283 ticks y 28 saltos.

El runner completo de campaña host pasó con el fixture Dragon: piso 0 (jefe y
salida), piso 1 (salida) y piso 2 (Goblin King y salida). Evidencia en
`_local/dragon-campaign-routes163-final/route-results.json`. La suite específica
del exportador pasó 20 pruebas y los 13 grupos C host pasaron; el Dragon se
mantiene fuera del paquete de entrega. Estos resultados no acreditan paridad
nativa ni ejecución Vita.

Se repitió el smoke desde una copia aislada basada en `C:\Vita3k`, pero Vita3K
0.2.1.4194 agotó 60 segundos antes de crear sesión nativa o logs. Resultado:
`_local/vita3k-dragon-anchor20-29-163/runs/iteration163/run-000001/result.json`.
La falla sigue ocurriendo antes de cargar/ejecutar el juego. Selección nativa
de sala/ancla, RNG y acciones completas del Dragon, Worm y máscara final de
jefes continúan pendientes. Véase
[DRAGON_ROUTE_DIAGNOSTIC163.md](DRAGON_ROUTE_DIAGNOSTIC163.md).

VitaSDK recompiló el código actual como VPK 01.63 de 843,322 bytes
(SHA-256 `1B0E48E2E9DBE7ECBA6EE0C3892B98C27FFB32CEF5CA55EEB8F94D463A6DFCFD`);
`check_vpk.py` valida sus siete entradas sin errores ni avisos. La suite sigue
en 206/206 Python y 13/13 grupos C. El primer intento de harness detectó proofs
con fingerprint antiguo; se regeneraron en `_local/campaign-163-currentproof`
y el smoke intro→piso 0 sí lanzó una copia aislada desde `C:\Vita3k`. Tras 60 s,
Vita3K 0.2.1.4194 expiró sin crear sesión nativa ni logs. Resultado:
`_local/vita3k-dragon-actions-163-currentproof/runs/iteration163/run-000001/result.json`.
El bloqueo del emulador sigue siendo previo a la ejecución del juego.
