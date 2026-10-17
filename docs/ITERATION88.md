# Iteración 088: selección de enemigos ACT_ONE

La desensamblación x86 de `LevelGenerator::addRandomEnemies` en
`0x0068d1e0` recupera la rama de selección para ACT_ONE. Después de los
filtros del candidato, el roll 0–99 asigna Goblin (ID 9) en 0–39, Slime
(ID 12) en 40–84 y Lurker (ID 11) en 85–99. El último caso pasa además por
un filtro de solidez vecina. Se corrigió el modelo provisional, que antes
codificaba ID 10 (Bat) en el tramo de Slime, y se cubrieron los límites con
pruebas.

El paquete privado `_local/delivery-act-one-campaign088-complete` contiene 8,
8 y 20 enemigos provisionales en los pisos 0–2: 19 Goblins, 15 Slimes y 2
Lurkers. El preflight valida 108 archivos, tres pisos y 56 imágenes. Los
hashes VWI2/VWE2 continúan coincidiendo con los proofs físicos de ruta seed
12345; solo cambian los VWE3. La suite Python pasa 165 tests.

Vita3K 00.86 se probó en el perfil dedicado con Vulkan y ventana visible. El
replay de campaña quedó preparado (545 pasos, 159 tiles de ruta), pero el
proceso no creó una sesión nativa, ventana ni log en 90 segundos. El resultado
está en `_local/vita3k-vagante-dedicated/runs/iteration086/run-000003`.
El VPK utilizado sigue siendo 00.86, SHA-256
`AD93FA7D5853499288CB94F835F5CE9393A721FC51FAADA23894B646587FDBFC`.

La selección usa el RNG provisional y la geometría que limita los Lurkers no
está completa. El orden nativo de candidatos, el jefe y las transiciones entre
pisos siguen pendientes. `floor_complete` y `campaign_complete` permanecen
falsos; no se verificó una ejecución de campaña en Vita3K ni en hardware.
