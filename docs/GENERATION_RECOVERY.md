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

## Recuperación 026: RNG y etapa de camino principal ACT_ONE

Se contrastó el código del PE local, no solo los tipos del pseudocódigo:

- 0x0067c539 llama seed 0x004ec020, que escribe el motor global 0x00b80c00.
- 0x0041cce0 lee límites globales en 0x00b81f88/0x00b81f8c. Los bytes
  iniciales del PE son 0 y 2147483647; devuelve MT19937 modulo 2^31.
  No equivale a tomar los 31 bits altos de la salida.
- 0x0067ccb6 y 0x0067cccf consumen dos enteros en ACT_ONE por intento.
  El segundo sustituye la dirección del primero. No simplificar a una llamada.
- Node 0x0067bdc0 inicia lados intentados por borde de región, orden
  izquierda/arriba/derecha/abajo. Se rota en ese orden para buscar lado libre.
- El conjunto de nodos visitados representa la pila activa. Al retroceder
  se retira el nodo; no es un conjunto permanente de celdas visitadas.
- Al alcanzar objetivo, se consume otro entero antes de materializar los enlaces
  recíprocos de la cadena de padres. Debe conservarse para etapas posteriores.

Las coordenadas de esta etapa usan cuadrícula 8×5. Para floorIndex 0, región
activa origen (2,0), tamaño 3×4, entrada Y=0, salida Y=3 y X aleatoria de 2 a 4.
Para floorIndex 1, origen (1,1), tamaño 5×3, entrada X=1 y salida X=5, Y aleatoria
1 a 3. Para floorIndex 2, origen (2,0), tamaño 3×5, entrada Y=4, salida Y=0, X
aleatoria 2 a 4. Cada caso consume dos valores para las coordenadas variables.
La separación mínima de tres ya se cumple con esas coordenadas fijas; no se
activa el bucle de reselección inicial en estos tres casos. La semilla aquí
es param_4 de generateLevel, no una semilla de campaña cuyo mapeo aún falta.

tools/floor_main_path.py reconstruye solamente esa etapa, emite enlaces de
habitaciones y conserva número de extracciones y hash de estado propio del RNG.
No emite VWI2, terreno, entidades ni un piso jugable. El hash usa la representación
canónica del MT del preparador, no un volcado de la disposición C++ de Windows.
Un límite de análisis de 100000 intentos aborta con error en vez de entregar un
resultado parcial; ese límite no se atribuye al juego original.

RoomDatabase::getRandomRoom 0x0075f250 selecciona el nivel de menor frecuencia,
conservando orden del vector, y toma random % cantidad. randomizeRooms
0x0075ed40 incrementa la frecuencia antes de comprobar geometría: también
cuentan intentos rechazados. choose_least_used implementa ese selector, pero
no llama aún al ensamblador ni reproduce sus reintentos, salas especiales o
variantes. Los constructores de Level/Room y generateDisplayIndices inspeccionados
entre selección de coordenadas y búsqueda no añaden consumos aleatorios para
estas habitaciones vacías; Room inicia UID vacío.

### Validación y alcance

Diecisiete pruebas Python pasan. Se comprobaron 300 caminos (100 semillas por
piso) con límites, ausencia de ciclos, extremos, enlaces recíprocos y reproducción
estable. Cinco pruebas nuevas también cubren referencia del RNG, distribución,
marcas de borde, parámetros y actualización de frecuencia de intentos.
El test independiente tests/mt_reference.cpp usa std::mt19937: coinciden 1000
valores por cada semilla 0, 1, 5489 y 4294967295, cruzando límites de bloque.
Esto valida la secuencia matemática del motor, no la salida completa del piso
original ejecutándose. Los informes indican pc_runtime_parity_verified=false.

```powershell
python tools/floor_main_path.py --seed 12345 --floor 0 --output _local/generation026/main-path-floor0-seed12345.json
python -m unittest discover -s tests -v
```

Para la semilla 12345 se obtuvieron caminos de 6, 7 y 7 habitaciones en pisos
0, 1 y 2, con 15, 15 y 35 extracciones. Es una muestra de la etapa reconstruida,
no un caso de equivalencia PC ni una nueva mazmorra instalada en Vita.

Pendiente inmediato: ramas y salas especiales posteriores al camino principal;
selección con comprobaciones horizontal/vertical; montaje de tiles y entidades;
entrada/salida y transformación final. Después debe conectarse al runtime y
probar transición desde INTRO conservando personaje. VPK vigente 025: no se
publica un ejecutable idéntico como revisión nueva mientras solo cambia el
preparador. Recuperación 026 se conserva aparte con fuente, informe y validación.
