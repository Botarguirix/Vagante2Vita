# Iteración 071: BreakableWall en ACT_ONE

El runtime ya registra `BreakableWall` (VWE2 kind 21) como entidad con
comportamiento. El constructor de Ghidra establece dos campos `float` en
14.0; corresponden a `Entity.hp` y `Entity.maxHp` según el layout PDB. La
función de daño delega en `Unit::hurt`, y al romperse llama
`Level::destroyTiles`, quita el tile y destruye sus paredes conectadas. El
runtime aplica daño de melee a 14 HP, enlaza paredes en la misma o en celdas
ortogonales, elimina tiles de tipo 1 o 6, limpia `midgroundIndex`, y actualiza
colisiones y autotiles. Como el terreno se entrega como imagen precompuesta,
una tile destruida se cubre con el color base de la cueva; sus capas de fondo
no se reconstruyen.

El renderer carga `entities/breakable_wall.png` de 16×16 y dibuja las paredes
que conservan HP. `prepare_world_bundle.py` ahora incluye el recurso en el
paquete ACT_ONE. La suite Python pasó 140 pruebas, incluido el empaquetado del
sprite. El código nuevo y la prueba C compilan para ARM mediante VitaSDK; esto
produjo `_local/build-act-one-071-final2/Vagante2Vita-iteration071.vpk`
(SHA-256 `CEEEB436A3634A0DD245A52347EAE36ABA2E574B166D5CCE2308C192A6147B2C`).
El paquete privado `_local/delivery-act-one-campaign071` contiene 68 archivos,
incluido el sprite, y pasa la validación de manifiesto.

Se ejecutó el harness cuatro veces sobre el clon aislado
`_local/vita3k-vagante-clone`; la última corrida usó el VPK indicado arriba,
Vulkan/SDL y el paquete privado 071. Cada proceso agotó 30 segundos y no creó
sesión nativa; `stdout.log`, `stderr.log`, `vita3k.log` y los logs de depuración
quedaron vacíos. Los intentos no validan la carga del juego ni el gameplay.

La prueba C de grupo conectado está añadida en `tests/world_entities_test.c`,
pero no se pudo ejecutar porque el entorno no tiene compilador C host; el build
VitaSDK verifica la compilación del runtime, no ejecuta ese test. El resultado
sigue siendo aproximado: todavía faltan los filtros completos de
`BreakableWall::hurt`, el sorteo de rotación y efectos, y el VWI2 no conserva
la marca de tile indestructible. El relleno visual plano pierde las capas de
fondo de la tile. Los Boulders independientes
(kind 22) permanecen pendientes y los mapas/enemigos continúan sin paridad
exacta; el primer piso no se declara completo.
