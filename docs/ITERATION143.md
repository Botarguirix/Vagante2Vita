# Iteración 01.43: export de jefe centrado y pruebas host Windows

El exportador ahora coloca el cuerpo de 8×8 píxeles del Goblin King en el
centro del tile de aparición. El tile `(28,21)` produce la posición `[456,344]`;
la posición anterior `[448,336]` dejaba al jugador sin poder continuar la ruta
después del combate. La prueba del exportador cubre el centro y la selección de
ancla cercana a la ruta.

`tools/run_act_one_campaign_routes_windows.ps1` compila el probe con
Clang/UCRT, corre los tres pisos, crea proofs VTR1 y registra el hash de las
fuentes. La huella coincide con `route_probe_fingerprint.py`, por lo que el
empaquetador puede validar los proofs. El runner rechaza una salida que no esté
vacía y verifica que Python y las herramientas de compilación estén disponibles.

Artefactos privados reproducibles:

- `_local/analysis143/routes/route-results.json`: los tres pisos pasan.
- `_local/analysis143/campaign-delivery/`: manifiesto válido de 142 archivos,
  incluye las tres proofs y trazas. `campaign_complete` queda en `false`.
- `_local/analysis143/campaign-replay.vts`: 494 pasos y 3,821 ticks nominales,
  construidos desde las trazas probadas.
- En el piso 2, Goblin King baja de 70 a 0 HP, el jugador conserva 41 HP y
  alcanza la salida en 2,161 ticks.

Validación: 201 pruebas Python con `python -m unittest discover -s tests -p
'test_*.py'`; 12 pruebas C host con `tools/run_host_tests_windows.ps1`; probe
compilado y campañas host repetidas correctamente. `git diff --check` no
reportó errores de whitespace. No cambió ni se reconstruyó el VPK porque el
cambio afecta al exportador de paquetes.

Vita3K sigue sin validación. La instancia 0.2.1.4190 clonada por completo y
aislada cierra con `0xC0000005` antes de crear la sesión y no escribe logs; los
únicos dumps encontrados son del 14 de octubre, anteriores a esta prueba. No
hay WinDbg/CDB/ProcDump disponible en PATH. El resultado del jefe y las
transiciones es evidencia del probe host, no del emulador. Persisten las
limitaciones de RNG de `addBoss`, combate/recompensa nativos y paridad completa
del generador. El runtime de tres pisos no se considera terminado.

## Revisión de instancia completa (17 de octubre)

Para atender la sospecha de una copia incompleta, se clonó el perfil aislado
completo en `_local/vita3k-act-one-fullcopy-143/instance`: 2,126 archivos,
358,187,492 bytes, incluyendo sistema, datos, cachés, firmware y Vagante.
`tools/clone_vita3k_profile_windows.ps1` permite repetir la copia y verificar
cada archivo por SHA-256; únicamente `runtime/config.yml` se modifica para
apuntar `pref-path` al filesystem del propio clon. El ejecutable Vita3K y el
`eboot.bin` instalado coinciden con el origen por SHA-256.

El clon completo volvió a fallar antes de abrir sesión nativa. El capturador
Windows de `_local/analysis143/vita3k_debug_capture.c` registró acceso de
lectura a dirección nula (`0xC0000005`, `RCX=0`) en `ucrtbase.dll`, durante el
frontend. La pila contiene `Vita3K.exe+0x683965`, `+0x6513cd`, `+0x15ef4c0` y
`+0x15e6859`; los dos intentos aislados reprodujeron los mismos offsets. La
distribución 0.2.1.4190 carece de símbolos y no hay evidencia suficiente para
mapearlos a funciones o culpar al VPK. Capturas completas:
`_local/analysis143/vita3k-debug-events.log` y
`_local/analysis143/vita3k-debug-fullcopy-143.log`; verificación del clon:
`_local/analysis143/vita3k-fullcopy-143.json`.

También se recompiló el código nativo con VitaSDK Windows al build privado
`_local/build-act-one-143/Vagante2Vita-iteration136.vpk`. El paquete tiene las
siete entradas esperadas, identidad `VAGV00006` / `Vagante2Vita` / `01.36`, y
hash SHA-256
`6F037C0995C9BB36E999AB2D24E7E39A16991D3B3C00E4E22225F7D669B37486`.
Una nueva instancia aislada instaló exactamente este VPK y validó la entrega
ACT_ONE de 142 archivos, 93 imágenes y los atlas de jefe antes del arranque.
El replay de campaña se preparó con 494 pasos; Vita3K no generó sesión nativa
en 30 segundos y stdout, stderr, vita3k.log y logs nativos quedaron vacíos.
Registro completo:
`_local/vita3k-vpk143-test/runs/iteration136/run-000001/`. Es otra prueba
fallida de arranque, no una prueba del contenido del VPK.

Los controles de arranque reducidos confirman que la barrera antecede a la
carga de Vagante: el modo consola con VPK por línea de comandos también agotó
30 s sin log en `_local/vita3k-vpk143-console-test/`. Sin pasar un título, SDL
y consola permanecieron activos 15 s sin crear log, stdout o stderr. Los
controles de `--version` y `--help` igualmente no imprimieron salida en cinco
segundos; esas opciones no se consideran por sí solas una falla funcional.
El mismo control SDL en el ejecutable local anterior (SHA-256
`054C3F54B871DEB7BF30A4796688A4CEABD572C562A66754FA66484628F34683`, distinto
del 4190) tuvo el mismo estado. Finalmente, lanzar la prueba de campaña con
`-Visible` tampoco creó ventana (`MainWindowHandle=0`) y agotó 45 s, con logs
vacíos. Los clones tienen perfiles propios; el proceso compartido Touhou no se
abrió ni se modificó. El patrón sitúa la barrera en la inicialización del
frontend/configuración de Vita3K en este entorno, sin demostrar si la causa es
Windows, el runtime empaquetado o la configuración local.

Revisión final: `python -m unittest discover -s tests -p 'test_*.py' -q`
ejecutó las 201 pruebas correctamente; `git diff --check` pasó (solo avisos
normales de conversión CRLF/LF). La entrega original de recursos usa contenido
del juego local, permanece fuera del VPK y fuera de Git.
