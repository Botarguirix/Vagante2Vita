# Iteración 028: interacción de cofre ACT_ONE

Ghidra indica que `Player::moveUp` abre el cofre tocado y que la caja creada por
`Chest` mide 12×9 píxeles. El jugador ahora abre un cofre cercano al pulsar ↑;
solo procesa el flanco de pulsación y no vuelve a abrirlo después del primer
cambio de estado. El estado abierto se conserva en memoria y cambia el sprite.
El loot no se inventa: `Chest::open` lo delega al `ItemGenerator`, que aún no
está conectado.

`tests/player_test.c` cubre el toque, apertura única y pulsación sostenida. La
fuente `player.c` compila para ARM con VitaSDK y `-Wall -Wextra -Werror`; las
103 pruebas Python pasan. El VPK 00.28 se creó en
`_local/build-act-one-028/Vagante2Vita-iteration028.vpk`, pesa 800989 bytes y
su SHA-256 es `3F2FC92A6C5284C61308DB9392E7C3DF040B13FCFC0D70928B88A568F57E9127`.
El archivo verifica como ZIP/SFO con identidad `VAGV00006 / Vagante2Vita / 00.28`.
Contiene eboot y archivos LiveArea únicamente; los recursos privados del juego
se mantienen en `_local` y se entregan externamente.

No hubo validación de gameplay en Vita3K. Tampoco se probó en hardware. La suite Python no ejecuta el
binario C; el test de jugador y `act-one-route-probe` se compilaron como
objetos ARM, pero no se enlazaron ni corrieron como ejecutables host. En esta
continuación se ejecutó el flujo Python con el runtime empaquetado de Codex:
106/106 pruebas pasan. `tools/build_vpk_windows.ps1 -Iteration 028` recompiló
el VPK (800989 bytes) con VitaSDK; no es prueba de gameplay. El probe de física
sigue pendiente: no hay CMake ni compilador C host instalados y `wsl.exe
--status` aún devuelve `E_ACCESSDENIED`. Se añadió `tools/run_act_one_route_probe.ps1`
para compilar/ejecutar el mismo probe con clang/gcc/cc cuando esté disponible.
En esta máquina se enlazó el probe actualizado como ELF32 ARM (127452 bytes);
no se ejecutó porque requiere Vita/emulador.

`test_act_one_seed_sweep.py` generó 20 seeds privadas (0–19) en los pisos ACT_ONE
0–2: los 60 mapas 80×40 pasan spawn geométrico, ruta de tiles y límite de
entidades. Esto prueba generación y conectividad, no física real; véase
[GENERATION_RECOVERY.md](GENERATION_RECOVERY.md).

Para completar el primer mundo aún hay que terminar generación y ensamblado
final del piso, integrar loot y estados de cofres, simular peligros y daño,
implementar enemigos/combate, salida y transición de pisos, y validar un
recorrido de extremo a extremo en Vita3K y Vita física.

La pasada nueva de Ghidra no produjo un decompilado adicional: el PDB completo
se atasca al aplicar sus tipos antes de ejecutar los scripts. La exportación
local previa sí contiene `addRandomEnemies` y `addEnemy`; permite identificar
las ramas ACT_ONE, pero `Room::monstersAllowed` queda como stub y no expone el
presupuesto necesario para reproducir spawns. Se documentó esta recuperación
parcial en [GENERATION_RECOVERY.md](GENERATION_RECOVERY.md); no se añadieron
enemigos ficticios al mapa. La suite actual vuelve a pasar 106/106. WSL sigue
bloqueado por `E_ACCESSDENIED`, por lo que el probe de física no se pudo enlazar
ni ejecutar como programa host en esta sesión.
Tres lanzamientos aislados del runtime —Vita3K 0.2.1.4183 con Vulkan y 0.2.1.4170
con Vulkan y OpenGL— se bloquearon antes de crear ventana o log. El Visor de
eventos de Windows registró en cada intento un error de lectura en dirección
nula dentro de Vita3K.exe; cambiar renderer no lo corrigió. Las ejecuciones
fallidas están en `_local/vita3k-vagante-clone/runs/iteration028/run-000001/`
y `_local/vita3k-vagante-4170/runs/iteration028/run-000001/` y
`run-000002/`. Los logs vacíos no permiten atribuirlo al VPK ni concluir nada
de gameplay: hay que recuperar primero un entorno de emulador que arranque.
El harness sincroniza DLL y recursos estáticos, incluidos plugins Qt, sin
compartir filesystem, preferencias ni cachés. No modifica `G:\Vita3k`.

## Qué falta para declarar ACT_ONE completo

El criterio de aceptación es construir un piso determinista desde una semilla,
ensamblar terreno/colisiones/agua/salida y entidades, y demostrar un recorrido
con `player_step` que abra cofres con loot, active trampas, combata al menos los
enemigos propios del piso, sobreviva o muera con reglas comprobadas y alcance
la salida. La salida debe crear el piso siguiente conservando estado/semilla.
Cada transición debe tener fixtures y pruebas host; al cerrar, una replay
automatizada debe validar el mismo recorrido en Vita3K funcional y en hardware.
Los sprites y datos comerciales se suministran aparte desde la copia del usuario.

El bloqueo inmediato es de validación, no evidencia de un bug del juego:
obtener un Vita3K que arranque en esta máquina o ejecutar la suite host mientras
se resuelve. En paralelo faltan paridad final del generador, agua/salida,
instanciación e interacción de entidades, loot, daño/enemigos, transición y
guardado mínimo. El ACT_ONE actual todavía declara `floor_complete=false`.
