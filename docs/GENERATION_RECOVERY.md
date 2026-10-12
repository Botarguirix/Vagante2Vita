# Recuperación de generación: evidencia y dependencias

Análisis de las funciones locales identificadas por PDB/Ghidra. El pseudocódigo
no se publica ni se presenta como código recompilable.

## Rutas verificadas

LevelGenerator::generateFloor, 0x0067c370: índices de piso 0–2 llaman generateLevel
con ACT_ONE, 3–5 ACT_TWO, 6–8 ACT_THREE; 9–11 generateRandomIslands; a partir
 de 12 generateFinalFight. Después llama randomizeLevelFeel. Los primeros
cuatro casos toman un valor aleatorio; el tipo de motor identificado es
std::mersenne_twister_engine de 32 bits, estado de 624 palabras (MT19937).
No se ha resuelto todavía el mapeo de distribución/semilla consumida.

LevelGenerator::generateBranch, 0x0067c450: pisos <3 Ruins, <6 Forest,
<9 Tower y restantes Factory; marca isBranch. No sustituye generateFloor.

## Dependencias para el primer piso real

- generateLevel 0x0067c4c0: ensamblado y selección de habitaciones/nodos.
- RoomDatabase::randomizeRooms 0x0075ed40 y getRandomRoom 0x0075f250:
  selección, frecuencia y orden de consumo del RNG.
- RoomDatabase::roomsFitVertically 0x0075eb90: conectividad vertical.
- addStartAndGoal 0x00688e30 e initLevel 0x0068a450: entradas/salidas y carga.
- addEdgeBoundary 0x0068a4b0 y generación de flags/display existentes:
  colisión y gráficos tras ensamblado.
- addTileEntities 0x0068ebf0, addRandomEnemies 0x0068d1e0,
  addRandomTraps 0x0068f1b0 y addBoss 0x0068a770: contenido jugable.

El runtime actual restringe World a 40×24 tiles y terreno 640×384. Debe admitir
mapas de otros tamaños, cámara y recursos por acto antes de conectar un piso
completo. Hay que conservar un caso determinista con semilla y validar
conectividad/spawn/salida. Esta recuperación de rutas no implica que los
pisos estén implementados ni que una galería de habitaciones sea generación.
