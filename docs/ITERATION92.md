# Iteración 092: ramas de superficie para enemigos ACT_ONE

Ghidra muestra que `LevelGenerator::addRandomEnemies` separa los candidatos
según `Level::isSolid(x, y - 1)`. Cuando el tile encima del candidato es
sólido, ACT_ONE usa
`uniform_int<int> % 125`: Bat (ID 10) en 0–49 y Spider (ID 14) en 50–69 si
`floorIndex > 0`; el resto no crea enemigo. Sin techo sólido usa el roll
ACT_ONE `% 100` recuperado: Goblin (ID 9) 0–39, Slime (ID 12) 40–84 y Lurker
(ID 11) 85–99, con los filtros geométricos de Lurker. El árbol de posiciones
G/H de BoulderTrap/Boulder decide qué rama evaluar; no representa tiles
prohibidos. La implementación provisional sigue sin reproducir los filtros
completos de entidades, los draws intercalados, la secuencia completa de hooks
ni todos los candidatos, así que no reclama paridad nativa.

La seed 12345 exporta 8, 8 y 20 enemigos provisionales para los pisos 0–2.
El piso 0 contiene 2 Bat, 3 Goblin y 3 Slime. VWI2 y VWE2 del piso 0 conservan
sus hashes del bundle anterior; cambia VWE3. El bundle privado
`_local/delivery-act-one-campaign092` valida 108 archivos, tres pisos y 61
imágenes. Pasan 168 pruebas Python.

VitaSDK Windows compiló VPK 00.92 (830,423 bytes; SHA-256
`6388FC5701100CEF1062E4F550317B199443128A1E907FEFBB3837CA7E699FF4`). La
replay automatizada contiene 545 segmentos, 3,184 ticks y rutas físicas
`player_step` en los tres pisos. Tres ejecuciones aisladas de Vita3K,
incluyendo dos con OpenGL y la última con recursos actualizados, agotaron 90
segundos antes de crear una sesión nativa; no hubo logs del runtime, stdout ni
stderr. El resultado tras actualizar VPK y recursos está en
`_local/vita3k-vagante-test092/runs/iteration092/run-000003/result.json`.
Esto no acredita gameplay en Vita3K.

`floor_complete` y `campaign_complete` siguen falsos. Faltan paridad completa
de generación, comportamiento de jefe, transiciones de campaña verificadas en
emulador/hardware y validación nativa del primer mundo.
