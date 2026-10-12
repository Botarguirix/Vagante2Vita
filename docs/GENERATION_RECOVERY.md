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

## Recuperación 024: dimensiones y conectividad de habitaciones

Level::Level 0x00646c70 distingue roomW/roomH de widthOfSingleRoom y
heightOfSingleRoom: w = roomW * widthOfSingleRoom y h = roomH * heightOfSingleRoom.
generateLevel inicia ACT_ONE con cuadrícula 8×5 y rooms.json aporta 10×8 tiles
por habitación: 80×40 tiles, 1280×640 píxeles de terreno. Por tanto el cargador
actual 40×24 no puede sustituirlo sin ampliar tamaño, cámara y caché.

Room::getRoomTypeBasedOnFlags 0x00756f00 confirma tamaño doble: bit 0x2000
para ancho y 0x1000 para alto. Room::load 0x007579d0 multiplica las dimensiones
por esas cantidades. Se validaron las 780 habitaciones de los 12 actos/grupos,
incluyendo entidades originales conservadas en la fuente privada. La nueva
herramienta no exporta ni modifica entidades ni monta pisos.

RoomDatabase::roomsFitVertically 0x0075eb90 se reconstruye como predicado
geométrico en tools/room_connectivity.py: compara salida inferior y entrada
superior, acceso desde escalera/plataforma o escalón lateral, espacio libre y
apoyo en la habitación inferior. Usa tipo 1 en sus pruebas de sólido, tal como
la función recuperada; no sustituir por world_solid (tipos 1 y 7). Es direccional,
no una prueba de mera coincidencia de huecos. randomizeRooms 0x0075ed40 invoca
esta función para habitaciones cuyo enlace UP (bit 2) está activo; DOWN es bit 8.

El informe privado _local/generation024/room-connectivity.json incluye pares
compatibles, hash de rooms.json y conteos por acto. ACT_ONE: 287 habitaciones;
19 035 pares simples con enlaces DOWN/UP, 16 119 aceptados por la geometría.
No incluye casos de habitaciones dobles, conectividad horizontal, selección
ponderada/frecuencias, consumo RNG ni recorribilidad global. No equivale a
paridad física con PC: requiere contraste del ensamblado y recorridos reales.

Comando reproducible (después de extraer la copia del usuario):

```powershell
python tools/room_connectivity.py _local/assets/rooms.json --output _local/generation024/room-connectivity.json
python -m unittest discover -s tests -v
```

Seis pruebas nuevas cubren dirección, escalera/plataforma, escalón, espacio
libre, apoyo vecino, bordes cerrados, dimensiones y campos inválidos; pasan las
12 pruebas Python incluyendo las seis del archivo. No cambió el ejecutable de
Vita: el VPK vigente sigue siendo 023, y no se presenta un VPK idéntico como
una nueva versión jugable. El siguiente cambio de runtime debe admitir 80×40
sin inflar toda la caché a escala de actos futuros, y después conectar el
ensamblado de habitaciones y la transición de INTRO preservando personaje.
