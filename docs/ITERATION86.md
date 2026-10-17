# Iteración 086: reproducir recorridos físicos en Windows

Se instaló MSYS2 UCRT64 con GCC host para ejecutar el mismo probe C que antes
solo podía correrse en WSL. `tools/run_act_one_route_probe.ps1` acepta el
compilador explícito; el build se realizó en `_local/host-tests086` con GCC
16.1.0 y `-std=c11 -O2 -Wall -Wextra -Werror`. El test de `session_log.c`
detectó la firma distinta de `mkdir` en Windows; ahora usa `mkdir(path)` bajo
`_WIN32` y conserva `mkdir(path, mode)` en POSIX/Vita.

Los 12 ejecutables de test C definidos por `vita/CMakeLists.txt` compilaron y
pasaron en Windows. `tools/run_act_one_route_probe.ps1` compiló el probe contra
las fuentes reales de mundo, entidades, combate, loot, jugador, atributos,
melee, arco y replay; los tres mapas del paquete privado
`_local/delivery-act-one-campaign085` alcanzaron la salida con seed 12345:

| Piso | Tiles | Saltos | Ticks | Salida |
|---|---:|---:|---:|---|
| 0 | 56 | 14 | 1741 | alcanzada |
| 1 | 64 | 13 | 736 | alcanzada |
| 2 | 39 | 0 | 375 | alcanzada |

Se generaron pruebas nuevas en `_local/host-tests086/routes`. En los tres
pisos coinciden con los proofs previos de `campaign.json` los ticks, saltos,
tiles, hashes del mundo y entidades y el hash de la traza VTR1. Esto demuestra
que las rutas físicas y las trazas actuales son reproducibles en Windows con
el código C del runtime.

La suite Python se ejecutó bajo Python 3.12.10 (usando pytest del runtime de
Codex): 162 pruebas y 66 subpruebas pasaron. El VPK `00.86` se compiló con
VitaSDK Windows, conserva el ID `VAGV00006`, contiene las siete entradas
esperadas y pasa la integridad ZIP. Su SHA-256 es
`AD93FA7D5853499288CB94F835F5CE9393A721FC51FAADA23894B646587FDBFC`.

ACT_ONE sigue incompleto. Los mapas entregados no prueban generación exacta
por semilla; pisos 1–2 aún incluyen reparaciones geométricas provisionales; la
IA y el inventario no tienen paridad completa; falta el jefe y la comprobación
de transición en Vita3K. El VPK 00.86 quedó preparado en el perfil dedicado y la
replay de campaña de 545 pasos se preparó, pero el intento `iteration086/run-000001`
terminó antes de abrir una ventana o crear sesión nativa; no produjo log del
emulador ni código de salida. La superficie de automatización no expuso la
ventana que el usuario inició manualmente. Los intentos anteriores 000007/000008
también expiraron antes de sesión nativa. No se afirma una prueba del VPK dentro
de Vita3K.
