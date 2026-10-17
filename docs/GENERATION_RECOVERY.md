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

## Recuperación 027: ramas y tienda ACT_ONE

Se añade tools/floor_room_graph.py sobre el estado RNG al final del camino
principal. La etapa de ramas usa orden de sorteos izquierda/derecha/arriba/abajo,
distinto del orden izquierda/arriba/derecha/abajo del camino. Sus bits de enlace
siguen siendo LEFT=1, UP=2, RIGHT=4, DOWN=8. Se preservan las llamadas: dos
coordenadas sobre toda la cuadrícula 8×5; dirección solo para fuente ya enlazada;
otro entero si el vecino de destino ya está enlazado. No se filtran primero
candidatos válidos: los sorteos fallidos también cuentan y consumen el RNG.

La comparación en 0x0067d84e–0x0067d86b confirma random % 20 < 7 para aceptar
un enlace a habitación ya conectada. Permite ciclos y también intentos sobre
un enlace que ya existe. Vecino nuevo incrementa numRooms solo si queda sitio
hasta ancho×alto de región activa. Los límites de región se comprueban antes
 de la probabilidad. 0x0067d3a7–0x0067d3c3 confirma que el bucle termina cuando
cumple el límite de intentos (área×4) y además alcanza al menos nueve habitaciones
o llega al máximo de región. No se fuerza llenar toda la región ni se sustituye
por un árbol sin ciclos. No se incluyen aquí las ramas específicas de otros actos.

La etapa de tienda elige una casilla no enlazada de la cuadrícula completa.
Necesita vecino horizontal conectado; prueba derecha antes que izquierda.
Añade los enlaces recíprocos y ShopRoom (0x100); los tipos resultantes son 260
(RIGHT+SHOP) o 257 (LEFT+SHOP), ambos con plantillas originales en ACT_ONE.
Genera una tienda. Se verifica la regla en el bloque 0x0067db2b–0x0067dcf8.
La exclusión de vecino treasureRoom está presente en el original; en este orden
los tesoros aún no están marcados. No se elige arte de tienda ni se inventa un NPC.

### Validación y siguiente etapa

22 pruebas Python pasan. Cinco nuevas cubren umbral de ciclos 6/7, vecino nuevo,
máximo de habitaciones, borde, fuente vacía sin sorteo de dirección, preferencia
horizontal de tienda y casos de grafo conectado. Se probaron 300 grafos (100
semillas en cada piso ACT_ONE): preservan todos los enlaces principales, extremos,
enlaces recíprocos, conectividad de las habitaciones enlazadas y conteo exacto
de extracciones. En esos 300 casos, todos los tipos solicitados existen en
rooms.json de la copia del usuario. Eso no prueba que las plantillas elegidas
sean compatibles geométricamente, ni que el grafo coincida con un piso PC real.
Se conserva pc_runtime_parity_verified=false y floor_complete=false.

```powershell
python tools/floor_room_graph.py --seed 12345 --floor 0 --output _local/generation027/room-graph-floor0-seed12345.json
python -m unittest discover -s tests -v
```

Semilla 12345: pisos 0/1/2 tienen respectivamente 9/9/10 habitaciones tras
ramas, más una tienda; consumen 356/156/178 valores hasta esta etapa. Tesoros,
pedestales, transformaciones de habitaciones, selección con reintentos y montaje
de tiles/entidades siguen pendientes. Los safeguards del preparador abortan con
error tras 100000 intentos; no se atribuye ese límite al juego original.

Se conserva recuperación 027 aparte. VPK vigente 025: no cambia el juego mostrado
en Vita3K ni se entrega un mapa parcial como si fuera el primer piso completo.

## Recuperación 028: tesoro y selector geométrico

Se añade tools/floor_treasure.py. Después de la tienda se coloca una sala de
tesoro horizontal (TREASURE 0x200 + LEFT/RIGHT), con vecino conectado que no
sea tienda, y prioridad derecha antes que izquierda. Se cuentan primero casillas
vacías con vecinos válidos; si no hay candidatos no se consume RNG para buscar.
La búsqueda empieza alrededor de la salida: radio X/Y=1, amplitud=2, offsets
aleatorios [-radio, radio-1], y coordenadas limitadas a cuadrícula 8×5.

0x0067dd88–0x0067ddc7 confirma conteo/exclusión de tiendas. 0x0067dea2–0x0067ded6
confirma que el radio aumenta cuando contador==radioX×radioY×2, después de
sortear la casilla y antes de validarla. El contador se reinicia, radio X crece
hasta 8 e Y hasta 5, amplitudes aumentan en dos. No se sustituye por selección
uniforme sobre una lista prefiltrada. La casilla de esa iteración conserva el
sorteo con el radio anterior. Los enlaces y flag treasureRoom se asignan en
0x0067df7e. Se preservan todas las extracciones, incluidas casillas repetidas,
ya enlazadas, recortadas al borde o descartadas por tienda.

También se añade tools/floor_room_selection.py con las reglas de salas simples
de randomizeRooms 0x0075ed40: recorre columnas y después filas; candidatos se
seleccionan por mínima frecuencia; cada elección incrementa frecuencia antes
de validación. Enlace izquierdo exige alguna apertura común entre bordes,
con tipo 1 como sólido; enlace superior usa roomsFitVertically recuperado.
La comprobación no se aplica si el borde no tiene ese enlace o está en el límite
de la cuadrícula. Tamaños dobles aún no admitidos por este selector.

Tras 21 elecciones incompatibles, el original conserva el último candidato y
registra advertencia: se reproduce la retención con geometry_compatible=false y
retained_after_retry_limit=true. No se declara válido por terminar el bucle.
Si falta el pool solicitado, este preparador falla explícitamente: aún no se
reproduce el fallback de habitación vacía del original. La función no monta
piso ni implementa su regeneración final al fallar recorribilidad global.

### Validación

29 pruebas Python pasan. Cuatro nuevas cubren exclusión de tienda, prioridad,
radio ampliado tras la tercera muestra inicial, cero sorteos si no hay
candidatos y 300 grafos con tesoro. Tres pruebas del selector cubren orden de
recorrido, abertura horizontal, soporte/altura vertical, frecuencia de intentos
rechazados y candidato retenido tras alcanzar límite.

Se probó el selector con 300 grafos y rooms.json originales: 12000 colocaciones;
ninguno de los 300 casos agotó reintentos con geometría incompatible. Solo
comprueba uniones locales de salas, no desplazamiento completo de un jugador.
La selección se ejecutó anticipadamente para probar el componente: falta
consumir antes las etapas de secretos y pedestales, por lo que esos UID no se
atribuyen a la salida final original con esa semilla. Informes privados marcan
analysis_only_early_selection=true y secret_and_pedestal_rng_not_consumed=true.
Los JSON de análisis con UID permanecen fuera de GitHub.

```powershell
python tools/floor_treasure.py --seed 12345 --floor 0 --output _local/generation028/treasure-floor0.json
python -m unittest discover -s tests -v
```

Semilla 12345: tesoros (2,2), (6,2), (2,0) para pisos 0/1/2; intentos 1/8/2.
No son mapas completos ni casos contrastados con ejecución PC. VPK vigente
025. Pendiente: randFloat y shuffle originales, secretos/pedestales, montaje
final de tiles/entidades y entrada/salida, recorribilidad global y transición.

## Recuperación 029: sorteos decimales, secretos y pedestales

tools/generation_random.py reproduce los adaptadores de esta compilación:
randFloat 0x0041ca10 llama generate_canonical en 0x0041c980. La instrucción
log2f(2^32), división de 24 por ese resultado y ceil determinan una extracción
MT. El uint32 se convierte a float32 antes de dividir por float32(2^32).
Se verificaron constantes PE: 0x00b204c4=24, 0x00b1ef4c=1,
0x00b20820=2^32, tabla double 0x00b21950/58=0/2^32.
No aparece una corrección hacia el anterior representable: UINT32_MAX redondea
a 2^32 y devuelve 1.0. No se sustituye por random.random ni por una biblioteca
moderna cuyo tratamiento del extremo o consumo pueda ser diferente.

shuffle 0x004e80e0 avanza i=1..N-1, sortea j en [0,i] y permuta ambos elementos,
incluyendo consumo para j==i. _Rng_from_urng 0x00446930 usa palabras completas
uint32 y rechazo de la cola sobrante antes de módulo. bound=1 consume cero;
una potencia de dos acepta UINT32_MAX. Esto difiere de game_int (low31).
Todas las etapas comparten el mismo MT y reanudan comprobando su estado.

tools/floor_special_rooms.py añade las dos etapas siguientes al tesoro.
0x0067e0d0–0x0067e131 confirma que se sortea 1 o 2 secretos con game_int,
se consume randFloat siempre y se pide cero si el float convertido a double
es menor que la constante double 0.33 (0x00b201c8).

addSecretRooms 0x0068f650 y su predicado 0x0068f8b0 enumeran casillas vacías
por columna, después fila. Vecinos: izquierda antes que derecha; deben tener
enlaces, no tener ya enlace hacia la casilla, ni flags tienda/tesoro/pedestal.
El act almacenado en Room (confirmado en constructor 0x00754870) añade
exclusión de brightSky solamente para act==1, no ACT_ONE==0.
Con pedido cero se vuelve inmediatamente, sin enumerar ni barajar candidatos.
En los demás casos se baraja toda la lista inicial, se toma del frente y se
vuelve a calcular/barajar sus vecinos antes de elegir el primero. Las casillas
restantes no se vuelven a enumerar; un secreto recién creado puede convertirse
en vecino de otro candidato. isSecret se conserva como metadato separado;
no es un bit que cambie el pool de room_type.

addBookPedestalRooms 0x0068fae0 obtiene tipos de la base que tienen BOOK=0x4000,
quitando ese bit con getAllTypesOfActWithBitSet 0x0075f7a0. Si no hay tipos,
no baraja; si los hay, baraja las 40 casillas en orden inicial columna/fila,
incluso si se pide cero. Después omite entrada y salida. Una sala conectada
puede recibir BOOK si su tipo actual está en el conjunto, y no tiene flags
tienda/tesoro/pedestal. No se excluye isSecret en esa comprobación.

Para casillas vacías se preservan dos comprobaciones asimétricas confirmadas
por ensamblador 0x0068fd33–0x0068fe65: el vecino derecho se intenta si existe
tipo base 2 en el conjunto; el vecino izquierdo si existe base 0. Las conexiones
resultantes son RIGHT=4 y LEFT=1 respectivamente. No se corrigen esos literales
por intuición. Se excluyen vecinos tienda/tesoro/pedestal. ACT_ONE privado tiene
base 2 y plantillas BOOK|4, de modo que la rama derecha puede crear una sala
adicional. La izquierda no se habilita con base 1. Si una futura base de datos
produce un tipo sin pool, el selector conserva su error explícito.

### Validación y alcance

42 pruebas Python pasan. Las 13 nuevas comprueban redondeo al extremo 1.0,
rechazo del uint32 máximo para bound=3, aceptación para bound=4, consumo de
auto-permutaciones, ambos lados de la frontera float32 próxima a 0.33,
pedidos cero, candidatos insuficientes, vecinos recalculados, exclusiones,
salas existentes, reglas literales de salas nuevas y enlaces recíprocos.
tests/generation_float_reference.cpp contrasta la conversión con compilador
C++ del host y std::mt19937: 4000 float32 coinciden bit a bit para semillas
0, 1, 5489 y UINT32_MAX. Esta referencia comprueba el adaptador numérico;
no ejecuta Vagante ni certifica paridad de un piso completo.

Se probaron 300 grafos (100 semillas × 3 pisos) con rooms.json del usuario:
12000 selecciones, cero fallos geométricos locales; máximo 12 intentos en una
casilla. Secretos colocados: cero en 95 casos, uno en 101 y dos en 104.
Pedestales: 172 en habitaciones existentes y 128 en nuevas horizontales.
Estos conteos no son una prueba de probabilidades estadísticas ni de que el
personaje pueda recorrer el piso. Los datos y UID quedan en informes privados.
La selección ahora consume primero secretos y pedestales; se retira la
limitación de ejecución anticipada de 028 para estos informes nuevos.

Semilla 12345, pisos 0/1/2: 1/0/2 secretos, un pedestal cada uno;
404/213/230 extracciones antes de selección, 444/253/271 después.
El informe mantiene floor_complete=false y pc_runtime_parity_verified=false.
Quedan initRoom y modificaciones de tiles, entidades, bordes, sellado de
secretos, agua, entrada/salida, validación global/regeneración y transición.
No se emite VWI ni se reemplaza la intro del VPK 025 con un grafo incompleto.

```powershell
python tools/floor_special_rooms.py _local/assets/rooms.json --seed 12345 --floor 0 --output _local/generation029/special-floor0.private.json
python -m unittest discover -s tests -v
```

La recuperación 029 conserva código propio, fuente, hashes y validación aparte.
El VPK instalable sigue siendo 025, con todos los recursos originales externos.

## Recuperación 030: montaje de tiles y ubicaciones de entidades

tools/floor_layout.py reanuda secretos/pedestales, selecciona los 40 UID por
las reglas anteriores y monta un grid privado 80×40, 3200 tiles, ACT_ONE.
Cada tile conserva sus seis campos JSON: tipo y cinco capas gráficas.
El traslado usa origen (roomX×10, roomY×8), como initRoom 0x00655be0.
Se copia por valor para que modificaciones de una instancia no alteren la
plantilla ni otras instancias del mismo UID. La rutina original también
propaga minLight y otros miembros no serializados: iluminación todavía
pendiente, no se afirma reconstrucción completa de Tile ni de initRoom.

addEdgeBoundary 0x0068a4b0 para ACT_ONE establece tipo 1 en la primera/última
fila y columna, conservando índices gráficos. Se recupera esa operación sin
inventar una habitación de relleno: incluso tipo 0 se obtiene de su pool
original. En las 300 muestras el borde ya era sólido y no cambió ningún tipo;
la prueba sintética abierta comprueba que el cierre funciona cuando hace falta.

Las posiciones declaradas en entidades se mantienen con sus variables y UID de
origen. Level::createRoomEntities 0x0064aaf0 recorre filas antes que columnas;
Room::createEntities 0x0075a830 suma origen en píxeles (room×dimensión×16) a
coordenadas float32 locales. La base contiene un Chest con Y=68.5: no se puede
convertir todo a int16 sin perder precisión. world_pixel usa la suma float32;
definition conserva los parámetros JSON originales. Esto prepara ubicaciones,
no construye inventario, trampas, cofres ni objetos aleatorios. El exportador y
runtime VWE2 añadido más adelante conserva ubicaciones y algunos parámetros;
no ejecuta esas entidades ni las convierte en comportamiento nativo.

### Candidatos de entrada: regla recuperada, ejecución final pendiente

tilePathsToEdgeOfRoom 0x0065b8a0 rechaza sólidos y el borde exterior del nivel,
y busca por cuatro vecinos no sólidos dentro de la habitación y un tile fuera
de su perímetro. Puede salir por un lado con enlace correspondiente; también
consulta bits alternativos 0x10/20/40/80. Se verificó el paso +/-1 en ensamblador
0x0065bbc0–0x0065bc2c. El helper se limita a salas simples y ACT_ONE cerrado;
no implementa tutorial, tamaños dobles ni mundos de cielo/laterales abiertos.
El flood mide conectividad de tiles; no simula tamaño, salto ni física de jugador.

findRandomAboveGroundTiles 0x0065a9a0 recorre columna/fila y requiere tile no
sólido, camino al enlace y soporte sólido debajo. Los tipos 1 y 7 son sólidos,
como generateCollisionFlags 0x00656cd0; plataforma 6 no cuenta como sólido.
Si el tile o sus vecinos horizontales son agua (9), se guarda en grupo aparte.
El grupo seco se prefiere cuando no está vacío; el original baraja solamente
el grupo elegido con shuffle recuperado en 029. Este análisis devuelve las
listas sin barajar y sin consumir RNG, porque esa llamada ocurre después de
display, sellado, agua y creación de entidades. No se atribuyen los candidatos
de esta etapa temprana a la entrada definitiva ni se genera un VWI con un
punto escogido arbitrariamente. La salida requiere otro predicado y ordenación.

### Pruebas y estado

48 pruebas Python pasan. Las seis nuevas cubren copia independiente de capas,
cierre sin borrar índices, orden/offset float32 de entidades con parámetros
anidados, rechazo de duplicados/dimensiones dobles/UID ausente/coordenadas
inválidas, cuatro enlaces y sus bits alternativos, componente encerrado,
agua cercana y plataforma que no sirve como soporte sólido.

Con la base original se montaron 300 layouts (100 semillas × 3 pisos),
12000 habitaciones y 960000 tiles. Todas las muestras tienen borde sólido,
ubicaciones dentro de 1280×640 píxeles y candidatos de entrada no vacíos.
298 muestras tienen candidatos secos y 2 solamente cerca de agua.
Ubicaciones acumuladas: Chest 3936, Spikes 4086, Doodad 1924, DartTrap 598,
BoulderTrap 406, Boulder 257, BreakableWall 163 y BoulderBlocker 13.
No se descarta ninguna categoría por no tener todavía lógica en Vita.
Semilla 12345: 44/29/45 ubicaciones de entidades para pisos 0/1/2.

Los JSON privados marcan entities_instantiated=false, secret_sealing_implemented=false,
water_simulated=false, entrance_exit_placed=false y floor_complete=false.
Faltan inicialización gráfica/iluminación, sellado con paredes rompibles,
agua, entidades funcionales, entrada/salida, prueba global y regeneración,
transición y juego completo. El archivo privado de layout contiene datos
originales y no se publica en GitHub ni se incluye en el VPK.

```powershell
python tools/floor_layout.py _local/assets/rooms.json --seed 12345 --floor 0 --output _local/generation030/floor0-layout.private.json
python -m unittest discover -s tests -v
```

Recuperación 030 se conserva aparte con fuente, resultados y hashes.
VPK instalable vigente 025; no se aprueba rendimiento físico a partir de estas
pruebas de Python ni se entrega el layout parcial como un primer piso jugable.

## Recuperación 031: etapas de terreno conectadas

`floor_layout.py` ahora continúa automáticamente después del ensamblado. Calcula
índices gráficos iniciales, modifica el terreno de las habitaciones secretas y
actualiza los índices en las áreas tocadas. La orientación de cada pared consume
el RNG recuperado. Después ejecuta 60 pasos de la geometría de agua y calcula
candidatos de entrada con la búsqueda de terreno conectado recuperada.

La simulación de agua se ejecuta sin sorteos de partículas porque no conocemos
el estado libre del pool original. Por eso el informe guarda masa, celdas activas
y hash del campo, y declara incompleta la secuencia RNG a partir de esa etapa.
El agua todavía no se dibuja ni se proyecta a tiles Vita. Las ubicaciones de
entidades continúan como datos; no se instancian enemigos, trampas u objetos.
Los candidatos de entrada quedan sin barajar y no equivalen a una colocación
definitiva ni a una prueba de movimiento del jugador.

```powershell
python tools/floor_layout.py _local/assets/rooms.json --seed 12345 --floor 0 --output _local/generation031/floor0-layout.private.json
```

La recuperación 031 amplía el análisis privado y no cambia el VPK 025. Sigue
pendiente integrar actores de pared y entidades, iluminación y vértices,
renderizado de agua, sorteo/colocación de entrada y salida, recorrido global,
regeneración de pisos fallidos y transición desde INTRO.

## Recuperación 032: agua y extremos provisionales

La etapa de agua guarda ahora las celdas activas en el informe privado. El
análisis Ghidra de `findRandomAboveGroundTiles` y
`findRandomReachableTilesFromCenter` permite obtener candidatos de entrada y
salida. Para el VWI se elige determinísticamente el primer candidato seco y el
primer candidato de salida que no esté bajo agua, aplicando el orden observado
del comparador. Se aplican las mutaciones de tile de entrada/salida, soporte,
eliminación de hongos y autotiles antes de exportar.

La elección sigue siendo provisional: faltan el barajado nativo, el estado del
pool de partículas que consume RNG, el filtrado por entidades ya instanciadas y
la comprobación de movimiento real del jugador. Si el centro queda sólido se
usa una casilla libre de seguridad para mantener el VWI cargable. El informe
conserva `floor_complete=false` y declara que no alcanza paridad de generación.
El exportador privado produce `act-one-world.vwi` y una composición visual con
el agua superpuesta; los activos comerciales permanecen bajo `_local/`.

Iteración 026 conecta ese VWI opcional al arranque como mundo ACT_ONE, dejando
el mundo INTRO como fallback. El paquete se compila en WSL/VitaSDK y el parser
nativo valida el mapa 80×40, la entrada (30,4) y la meta (36,28). Las pruebas
locales pasan, pero la sesión automatizada Vita3K no pudo iniciar: el proceso
del emulador termina con `0xC0000005` antes de escribir el log. El mismo fallo
se reproduce con el VPK de control 025 usando runtime y perfil aislados, sin
otro proceso Vita3K visible; por eso no se atribuye al mapa nuevo. El harness
vacía `vita3k.log` antes de cada lanzamiento para no reutilizar evidencia vieja.
La copia privada dedicada está en `_local/vita3k-vagante-clone`; el smoke real
de ACT_ONE sigue pendiente de resolver el inicio del emulador.

El VPK 026 se mantiene privado en `_local/build-iteration026-wsl/` y contiene
código y recursos propios, no los assets extraídos. La carga del paquete por
el parser host prueba formato y datos, no una sesión jugable en Vita3K ni en
hardware. Siguientes pendientes: instanciar entidades, recuperar la prueba de
recorrido y RNG final, y completar transición/regeneración antes de declarar
terminado el primer mundo.

## Recuperación 033: geometría de spawn del jugador Vita

El exportador ya rechaza un layout si no declara validada la geometría de
aparición del runtime. `floor_layout.py` aplica el tamaño nativo actual de
8×12 píxeles, inset horizontal de 4 píxeles, tile de entrada libre y soporte
sólido o plataforma debajo. Para semilla 12345/floor 0, el spawn (30,4) queda
libre con soporte tipo 1. Esto verifica que `player_spawn` no empiece dentro de
una pared y que sus pies tengan un tile donde caer; no verifica desplazamiento
hasta la salida, física equivalente al Vagante original ni el resto de clases.

La suite Python suma una prueba de rechazo para paquetes cuyo spawn no esté
validado. Se regeneró el layout y VWI en `_local/generation033/`; conserva
80×40 y extremos (30,4)/(36,28), y `floor_complete` permanece falso.

## Recuperación 034: filtro A* nativo de entrada a salida

Ghidra documenta la búsqueda `orthogonalAStarTest` posterior a
`addStartAndGoal`: clampa ambos extremos, usa coste unitario y distancia
Manhattan, elige el mínimo `g+h` recorriendo el set en orden lexicográfico de
coordenadas y corta al alcanzar 5000 visitados. La búsqueda admite tiles tipo
6 porque solo excluye sólidos 1/7; es la validación topológica original, no una
prueba de movimiento.

`floor_tile_path.py` implementa esa auditoría y devuelve ruta, cantidad de
tiles visitados y si se alcanzó el límite. El generador adjunta el resultado y
el exportador rechaza layouts desconectados o sin esta comprobación. El layout
privado de semilla 12345 pasa con una ruta de 58 pasos y 291 tiles visitados.
El VWI conserva los hashes de 032 porque el cambio agrega evidencia de ruta,
no cambia sus tiles. Esto no reemplaza la simulación física del jugador: el
primer mundo sigue incompleto hasta verificar entrada, recorrido, entidades y
salida en el runtime.

## Recuperación 035: cobertura completa de la caja de spawn

La validación geométrica usa ahora la fórmula de `player_spawn` en
`vita/src/player.c`: caja de 8×12 px con origen `(tile_x*16+4,
tile_y*16+4)`. El auditor enumera cada tile que cruza la caja y cada tile bajo
los pies; no da por libre una sola celda por estarlo el punto de entrada. Una
regresión sintética desplaza la caja hasta cruzar la fila siguiente y confirma
que una pared en esa fila hace fallar `clear`.

En la semilla 12345 el cuerpo queda en (30,4) y se apoya en (30,5), tipo 1.
La regeneración privada de 036 conserva la ruta A* de 58 pasos y 291 tiles. El
VWI y el PNG mantienen los hashes de 034 porque la geometría auditada coincide
con la fórmula efectiva de spawn. Suite Python: 91/91.

El exportador vuelve a calcular esa caja y sus tiles de apoyo directamente
contra la matriz que serializa; rechaza evidencia vieja o una matriz alterada
después de generar el informe. `player-test` host cargó el VWI regenerado,
creó al jugador y completó su secuencia de 300 frames sin solaparse con sólidos.
Esta secuencia verifica inicio y colisiones bajo el movimiento implementado,
pero no guía al jugador hasta la meta ni valida que el mundo sea terminable.
Suite Python actual: 92/92.

## Recuperación 036: salida del mundo y bloqueo de Vita3K

La exportación VWI conserva la coordenada de meta, pero la lectura de
`main.c` confirma que el runtime no detecta contacto con ella ni dispara una
transición. El pseudocódigo guardado de Ghidra para
`Level::updateEntranceAndExitBasedOnMidground` (00657f70) encuentra los
marcadores midground 0/1, los registra y cambia el tipo de tile; el ensamblado
ACT_ONE actual no contiene esos marcadores antes de elegir extremos. La salida
no se marcará como funcional hasta recuperar el comportamiento que conecta
`ExitDestination`, el jugador y el cambio de piso.

El ejecutable local indicado para Vita3K (versión 0.2.1.4183) termina con
`0xC0000005` también al invocarlo con `--help` y `--version`, antes de leer un
VPK. La instancia aislada usa el mismo hash SHA-256 que ese ejecutable. En el
registro de Windows existen fallos previos del ejecutable bajo `G:\Vita3k`,
también dentro de `Vita3K.exe`; no se atribuye el fallo a ACT_ONE ni a la
compilación mientras el propio emulador no pueda iniciar. La automatización de
Vita3K sigue pendiente.

## Runtime: detector de salida y smoke ACT_ONE (código aún sin compilar)

Se añadió `world_goal_touched`, que valida valores finitos y prueba la
intersección entre la caja actual de 8×12 píxeles del jugador y el tile de meta
de VWI2. El loop registra `WORLD_EXIT` al contacto. Para conservar una ruta
jugable antes de tener el siguiente generador, el prototipo vuelve a aparecer
en la entrada del mismo mundo y etiqueta el evento como `exit-restart`; no es
un cambio de piso y no cuenta como mundo completo.

`act-one-export.json` conserva ahora la ruta A* completa y
`tools/prepare_smoke_replay.py --act-one-exit --layout <layout-o-export>` la
convierte en un replay VTS1 de 32 segmentos para la semilla actual. El replay
arranca ACT_ONE y camina/salta siguiendo los cambios de dirección; el evento
de contacto real sigue siendo el criterio de éxito. `tools/test_vita3k.ps1
-ActOneExitSmoke` y `--require-act-one-exit` validan carga, movimiento, salto,
contacto y cierre del replay. Este camino depende del mundo semilla 12345 y de
la posición actual del objetivo. Solo se comprobó la generación del archivo
VTS1 en Windows. No se compiló el C nuevo ni se ejecutó en Vita3K: WSL responde
`E_ACCESSDENIED`, CMake no está disponible en el PATH de Windows, y la instancia
Vita3K aislada falla antes de leer un VPK. La siguiente evidencia necesaria es
compilar `player-test`/VPK, confirmar que la ruta física realmente alcanza el
objetivo, y después sustituir el reinicio temporal por generación y transición
al siguiente piso.

## Paquete y sprites de entidades ACT_ONE (iteraciones 026–028)

El exportador genera `act-one-entities.vwe` en formato VWE2, con registro fijo
de 16 bytes: clase, flags, variante, coordenadas Q8.8 y ordinal de origen. Para
la semilla 12345 contiene 44 instancias: 12 cofres, 22 pinchos, 8 adornos, una
trampa de roca y una trampa de dardos. Se conservan coordenadas fraccionarias;
el cargador valida tipos, opciones, límites del mapa y tamaño del registro.
ACT_ONE requiere VWE2 para iniciar y retrocede a INTRO si falta o no valida;
INTRO conserva compatibilidad con VWE1.

Las instancias se guardan en `World.entities` y aparecen en el log. El bundle
copia PNG privados para los tipos presentes; el renderer dibuja cofres,
pinchos, trampa de dardos y doodads conocidos. Los atlas usan dimensiones
recuperadas en `Doodad::setTypeVars`, aunque algunos quedan estáticos o con
animación aproximada. Iteración 028 corrigió la interacción del cofre: `↑` al
tocar el cuerpo 12×9 px, según `Player::moveUp` y `Chest::Chest` en Ghidra. Una
nueva pulsación cambia el estado visual abierto una sola vez. Bloqueo y loot
siguen pendientes; `Chest::open` delega recompensas al `ItemGenerator`. También
faltan comportamiento de trampas, daño y físicas de entidades.

El exportador incluye explícitamente `floor_complete`, `entities_instantiated`,
`native_player_route_verified` y `native_exit_transition_verified`. Rechaza una
afirmación `floor_complete=true` si faltan paridad exacta, instancias, ruta
física verificada o transición real. El layout de la semilla 12345 conserva una
ruta A* de 58 pasos que declara `models_player_physics=false`; solo verifica
conectividad de tiles. Mantiene `exact_generation_parity=false` y 44 entidades.
La suite Python pasa 103/103. El bundle privado actual
`_local/world-data045/act-one` valida 54 archivos e incluye el informe de
exportación con hashes coincidentes. `vita/src/act_one_route_probe.c` añade una
simulación de ruta con `player_step` al build host y compila para ARM, pero su
ejecución queda pendiente porque no hay compilador host y WSL responde
`E_ACCESSDENIED`. Vita3K 0.2.1.4183 tampoco inicia ni con `--version`; no hay
validación de gameplay en emulador ni hardware. ACT_ONE sigue incompleto.
## Auditoría de catálogo de entidades ACT_ONE

La copia privada de `rooms.json` declara ocho tipos de entidad de habitación:
289 cofres, 258 pinchos, 50 trampas de dardos, 33 trampas de roca, 16 muros
rompibles, 15 rocas, un bloqueador de roca y 39 doodads. El exportador ahora
valida sus parámetros requeridos y falla ante tipos desconocidos o definiciones
incompletas. Una prueba local audita el catálogo completo sin guardar recursos
comerciales en Git.

Esto cubre solo entidades colocadas en plantillas. Ghidra confirma que
`generateLevel` llama a `createRoomEntities`, coloca entrada/salida, comprueba
el camino, luego ejecuta `addRandomEnemies` y `addTileEntities`, y al final
`removeUnfairEntities`. `addRandomEnemies` tiene advertencia de propagación de
tipos no convergente; `Room::monstersAllowed` se exportó sin cuerpo útil. Por
tanto, no se han generado enemigos en el runtime ni se afirma equivalencia de
spawns. Recuperar esos filtros y las rutinas de `Monster` es requisito para
conectar combate con el piso.

### Recuperación enfocada de enemigos (2026-10-15)

La exportación local de Ghidra `_local/decompiled/0068d1e0.c` conserva el cuerpo
de `LevelGenerator::addRandomEnemies` en `0x0068d1e0`, pese a la advertencia de
propagación de tipos. Es pseudocódigo x86 del juego Windows, no fuente C++ lista
para reutilizar. `Room::monstersAllowed` (`0x0075af50`) sigue reducido a un stub,
por lo que las reglas de elegibilidad por piso/NG+ de cada sala no se pueden
afirmar todavía.

El cuerpo recuperado muestra estas reglas ACT_ONE para celdas que ya pasaron
los filtros anteriores: se omiten salas tienda, se comprueba `tilePathsToEdgeOfRoom`
y la cercanía a la entrada; en los tiles de suelo sólido bajo una cavidad,
`rng % 100` selecciona Goblin (0–39), Bat (40–84), o Lurker (85–99), aunque
Lurker requiere geometría libre adicional. En otras celdas elegibles, la tabla
ACT_ONE observada elige Goblin (0–39), Slime (40–84), o Lurker (85–99), también
con comprobaciones geométricas para Lurker. `addEnemy` (`0x0068c4d0`) confirma
que IDs 9/10/11/12 corresponden a Goblin/Bat/Lurker/Slime. La llamada usa un
presupuesto de sala (`monstersAllowed`) y lo reduce por `Monster::getSpawnWeight`;
sin el retorno de ese cálculo, los conteos no se pueden reproducir exactamente.

La tabla `MONSTERS_ALLOWED_PER_ACT` (`0x00404c50`) sí permite recuperar la
lista de candidatos: la clave ACT_ONE=0 contiene IDs 9/10/11 (Goblin, Bat,
Lurker), y el inicializador vecino de clave 1 contiene 10/26 (Bat/Spider).
La tabla `MONSTER_SPAWN_WEIGHT` (`0x004035f0`) asigna peso 1.0 a Goblin, Slime,
Lurker y Bat. La inicialización de listas se decodificó del pseudocódigo local;
la rutina de elegibilidad de sala sigue sin cuerpo.

La iteración 029 añade un primer pase explícitamente provisional, no equivalente
al juego original: hasta un enemigo por sala simple ACT_ONE, excluye el cuarto
de boss y vecinos de entrada/salida, prueba terreno y camino hasta el borde,
y usa la distribución Goblin/Bat/Lurker observada en `addRandomEnemies`.
`act-one-enemies.vwe` (VWE3) transporta estas instancias separadas de VWE2.
El runtime les da movimiento de persecución sencillo, colisión básica y daño
por geometría del arma; sus cuerpos son marcadores de color y todavía no usan
las animaciones, ataques, salud, drops ni scripts originales. El exportador marca
cada spawn `native_parity=false`, por lo que `floor_complete` permanece falso.

La pasada de Ghidra de 2026-10-15 intentó reimportar el ejecutable y reconstruir
el PDB de 64 MiB antes de ejecutar el pre-script; el análisis del PDB se quedó
en una aplicación de cientos de miles de tipos. La investigación se retomó con
la exportación existente, sin modificar el proyecto Ghidra compartido. El
la paridad nativa de enemigos requiere reconstruir selectivamente
`Room::monstersAllowed`, `addEnemy` y la física/daño de los enemigos ACT_ONE.
La iteración 029 añade un prototipo separado para facilitar las pruebas, pero
no reemplaza esa recuperación ni se considera un mundo completo.

## Barrido reproducible de semillas

`tests/test_act_one_seed_sweep.py` genera ACT_ONE floor 0–2 para las semillas
0–19 con la base privada local. En esta máquina, los 60/60 layouts ensamblaron
80×40 tiles, presentaron spawn limpio y soportado, ruta de tiles alcanzable y
menos de 256 entidades. Es evidencia
del ensamblador y el chequeo geométrico de tiles, no de la física `player_step`,
paridad exacta con el juego original, entidades activas ni combate. Todos los
informes mantienen `floor_complete=false`.

## Ruta de recompensa de cofres recuperada

La exportación Ghidra ya incluye `Chest::finishOpen` (`0x00546210`),
`ItemGenerator::generateNewRandomChestItem` (`0x006434f0`),
`ItemGenerator::getRandomItemTypeFromPool` (`0x00645540`),
`Item::Item` (`0x00616000`), `Item::pickUp` (`0x00628460`) e
`Inventory::pickUp` (`0x00609cc0`). El enum PDB fija `POOL_CHEST=23`; el
constructor de `ItemGenerator` asigna a ese pool cuatro subpools: armas,
armadura, otros y varitas. `finishOpen` genera un objeto y lo añade al mundo;
el constructor de `Item` fija el cuerpo del objeto en 7×7 px. El inventario
comprueba si el objeto se puede recoger y luego busca una ranura compatible.

La tabla superior sí se pudo recuperar del listado de instrucciones del
constructor (`_local/item-generator-ctor-listing.txt`) y la copia local del
ejecutable: antes de insertar el pool 23, las llamadas a
`ItemPoolRarity::ItemPoolRarity` (`0x0063cd10`) cargan los tipos 15, 6, 22 y 11
con pesos float32 30, 60, 4 y 6 desde `.rdata` (`0x00b204fc`, `0x00b205a4`,
`0x00b203b4`, `0x00b203fc`). El enum PDB resuelve esos tipos como
`POOL_WEAPONS`, `POOL_ARMOR`, `POOL_OTHER` y `POOL_WANDS`, respectivamente.
Estos son pesos relativos (suman 100), no probabilidades ya normalizadas ni
objetos concretos. La selección descendente sigue dependiendo de los subpools
y niveles de objeto.

También se resolvió `POOL_WEAPONS` (15): su vector contiene
`POOL_SWORDS` (8), `POOL_DAGGERS` (7), `POOL_CLUBS` (9), `POOL_BOWS` (10),
`POOL_FIST` (13), `POOL_AXES` (14) y `POOL_RODS` (12). Cada entrada carga
`1.0f` desde `0x00b1ef4c`, así que estas siete familias tienen el mismo peso
relativo. La lista está construida entre `0x00641cde` y `0x00641d6c` y se
asigna al pool 15 en `0x00641dbc`.

`POOL_ARMOR` (6) también quedó recuperado: amuletos (5) pesan `0.7`, botas
(3) `1.0`, petos (1) `1.1`, guantes (2) `1.0`, cascos (0) `1.0` y anillos
(4) `0.75`. El vector se construye desde `0x00641df8` y se asigna al pool 6
en `0x00641ebb`. Los float32 se obtuvieron de `.rdata`; por eso el verificador
usa tolerancia de `1e-6` al compararlos.

El subpool `POOL_SWORDS` (8) contiene cinco registros de `ItemRarity`, también
verificados en la listing y los datos del PE: `SWORD` (49), nivel mínimo 0,
máximo 3, peso 1; `SWORD2` (50), mínimo 0, máximo 99999, peso 1;
`SWORD3` (51), mínimo 2, máximo 99999, peso 1; `SWORD4` (52), mínimo 4,
máximo 99999, peso 1; y `SWORD5` (53), mínimo 6, máximo 99999, peso 1.
El primer registro de cuatro enteros está en `.rdata` (`0x00b20fb0`); los otros
cuatro se escriben como inmediatos entre `0x0063f7f4` y `0x0063f8ae`. El
constructor de `ItemRarity` (`0x0063cc40`) asigna `maxItemLevel=99999` cuando
recibe la variante de tres enteros, y convierte el entero central a peso float.
El verificador ahora confirma esas cinco combinaciones además de las tablas de
categorías.

El verificador reproducible `tools/audit_chest_pool.py` compara los inmediatos
de estas tres tablas con los float32 del PE y rechaza cambios en el orden, los
IDs o los pesos esperados. Se ejecuta contra la copia privada analizada; no
extrae ni empaqueta recursos del juego.

Esto ya define el flujo que debe seguir la implementación, pero aún no produce
loot en ACT_ONE. Falta resolver pesos/raridades/niveles y todos los tipos
concretos de cada subpool desde el constructor (su pseudocódigo conserva otros
valores en registros no propagados), mapear sus sprites y atributos, y portar
el límite de capacidad, stacks y recogida del inventario. Hasta que esos datos
se validen, un premio fijo o aleatorio inventado sería una regresión de
fidelidad. El runtime solo registra que el cofre se abrió y conserva el arte
abierto.

### Comprobación de objetos colocados e inventario

Se recorrió la copia privada `_local/assets/rooms.json` por acto y sala. No hay
entidades `Item` colocadas en los pools de ACT_ONE; los siete registros que sí
aparecen pertenecen al acto índice 8, una sala especial con objetos ligados a
su guion. No se deben reutilizar como premio de cofre de ACT_ONE. Por tanto, el
primer objeto recogible del mundo debe proceder del generador de cofres y no de
una definición fija ajena a ese acto.

`Inventory::pickUp` (`0x00609cc0`) recorre primero el inventario de 7 columnas
por 5 filas; si no encuentra ranura vacía, informa que no hay espacio. Antes
comprueba duplicados para tipos únicos y delega los stacks en `doStackItem`.
`Item::pickUp` (`0x00628460`) valida `isPickupable`, desvincula el objeto de un
pedestal o tienda cuando aplica y lo entrega al jugador. Esto confirma que el
siguiente trabajo de runtime no es solo dibujar una caída: requiere conservar
tipo, cantidad y atributos; validar elegibilidad y espacio; y actualizar el
arma equipada mediante la definición de combate correspondiente. El conjunto
de definiciones cuerpo/ataque actual solo cubre las tres armas iniciales, así
que todavía no permite equipar con fidelidad todas las espadas del pool.

El auditor `tools/audit_chest_pool.py` confirma además las filas de dagas,
armas de puño, arcos y mazas desde las instrucciones del constructor de
`ItemGenerator` en Ghidra. El ID 62 del pool de mazas es `ITEMTYPE_FLAIL`
(nivel mínimo 1, peso 2), según el enum del PDB y la propiedad `flail` de los
datos locales de combate. No se agrega todavía a VWA porque usa `player-swing`,
que necesita su atlas y secuencia de frames propios. También se verifican las
categorías de cofres, armas, armaduras, las cinco espadas y las dos filas del
pool de hachas, incluida su primera fila estática en el ejecutable. La
comprobación usa el ejecutable y el listing locales:

```powershell
python tools/audit_chest_pool.py --exe _local/input/vagante.exe --listing _local/item-generator-ctor-listing.txt
```

La herramienta local `tools/extract_item_pools.py` ya asigna las 170 filas
`ItemRarity` del listing a sus pools por el orden de llamadas de Ghidra y
recupera los registros estáticos del PE. Comprueba que todas las filas y todos
los pools del constructor queden contabilizados. En la copia analizada se
recuperan 18 tablas directas, incluidas varitas y varas. El JSON de salida
permanece en `_local/`; el verificador necesita la copia propia del ejecutable,
el listing, el pseudocódigo y el TSV de enums exportados por Ghidra. Las tablas
ya no son el bloqueo principal: falta conectarlas a la selección jerárquica,
atributos de `Item`, entidad física, recogida y equipamiento en el runtime.

`tools/extract_item_sprites.py` relaciona esos tipos con los `spriteIndex` de
`Item::setNameAndDescription` (`0x00617290`) y con las hojas que la función
selecciona (`items-weapons`, armadura, pociones, pergaminos, libros y objetos).
La verificación cubre los 170 tipos nombrados de las tablas directas: 136
tienen índice explícito y 34 conservan el valor inicial `-1` en esa función.
Este campo no basta por sí solo para dibujar el objeto: aún hay que confirmar
cómo `Sprite` interpreta `frame=-1` y los índices con el atlas de 20 px y portar
la animación/caída del objeto. La salida permanece en `_local/`.

La Iteración 036 habilita el bit `facingLeft` en VWE2 para `DartTrap`, lo
serializa junto a `canReload` y refleja el sprite al cargarlo. Las plantillas
de habitación no proporcionan orientación explícita, así que el exportador
usa el default del constructor; `hasFired` sigue fuera del paquete. La
iteración 040 añade el movimiento, bloqueo por terreno e impacto aproximados
del proyectil; no reproducen aún todas las guardas de activación nativas.

## Evidencia de daño de pinchos

También se revisaron `Spikes::Spikes` (`0x007b2800`) y
`Spikes::doCollision` (`0x007b2c60`). El constructor habilita el pincho según
el tile de terreno tipo 11 bajo su entidad. La colisión exige que el actor
supere una condición de velocidad vertical de 0.4 y otras comprobaciones de
contacto; entonces construye `DamageEventMeta(999, DAMAGE_PURE, true, ...)`.
El enum PDB define `DAMAGE_PURE=1`. Esto permite implementar el peligro como
daño letal basado en la colisión descendente, en vez de inventar daño periódico
por simple proximidad. Aún falta portar las demás guardas del actor (inmunidad,
agua, estado de muerte/resurrección y registro de víctimas por pincho) y
conectar `Player::hurt`/resultado de muerte con el loop del runtime; el
pseudocódigo Ghidra no prueba por sí solo el resultado visual del golpe.
La primera integración está en [ITERATION30.md](ITERATION30.md); el test C y el
smoke Vita3K están compilado y lanzado respectivamente, pero no ejecutados con
resultado de gameplay comprobado.

## Investigación enfocada de la trampa de dardos

La exportación de Ghidra contiene cuerpos utilizables para inspeccionar
`DartTrap::DartTrap` (`0x00593040`), `DartTrap::doCollision` (`0x00593a50`),
`DartTrap::step` (`0x00593530`) y `DartTrap::fireDart` (`0x00594560`). El
constructor crea una entidad de 16×16, inicialmente `hasFired=true`, sin
recarga. `doCollision` no dispara si ya se activó; comprueba el movimiento del
actor, evita el tile de agua (tipo 11), requiere espacio libre en la dirección
de disparo y entonces invoca `fireDart`. `step` elimina la trampa no recargable
si el tile que la soporta deja de ser sólido. `fireDart` crea un proyectil del
faction jugador, orientado horizontalmente según `facingLeft`, y configura el
estado de activación para no repetir el disparo.

La inspección adicional recuperó `Projectile::Projectile` (`0x0071ff10`),
`Projectile::step` (`0x00721fe0`), `Projectile::doCollision` (`0x00725140`) y
`Projectile::doGenericArrowMovement` (`0x0072f7b0`). El constructor establece
el cuerpo del proyectil en 6×6; `step` despacha `PROJECTILE_ARROW_TRAP` por el
movimiento genérico de flecha, y la colisión agrupa ese tipo con flechas
normales para generar daño desde los campos del proyectil.

La integración actual de kind 18 está en `vita/src/world_combat.c`: usa
velocidad horizontal 5 px/tick, cuerpo 6×6 y daño físico 5, con impacto en el
jugador, bloqueo por tiles sólidos y recarga de 130 ticks. La prueba en
`tests/world_entities_test.c` fija la caja 6×6: el dardo no golpea antes de
solapar al jugador, sí lo golpea al cruzar su caja y activa la recarga. Esto es
una recuperación verificable de los parámetros y del movimiento, no paridad
completa: el disparador aún aproxima las guardas de movimiento/agua/espacio
libre de `DartTrap::doCollision`. El golpe ya descuenta salud en el loop del
jugador, pero siguen pendientes las guardas de faction e inmunidad por actor
que limitan la colisión original. El build de Windows puede usar
`tools/build_vpk_windows.ps1` con VitaSDK instalado; el camino WSL sigue siendo
preferible cuando el servicio WSL vuelve a estar disponible.
La iteración 054 amplía la tabla VWA generada desde el `attacks.json` privado a
12 armas con secuencia `player-stab`: dagas 45–47, espadas 49–53, vara 79 y
CQC 85–87. `Item::getWeapon` (`0x00626920`) confirma sus nombres de propiedad;
el encoder conserva frames, polígonos de golpe, daño y knockback. Las mazas y
hachas usan otras animaciones; el ataque pesado ocupa celdas de 32×32 y la
guion de maza referencia el sprite 9, fuera del atlas de 3×3 que carga el renderer actual.
No se incluyen hasta adaptar sus atlas y dimensiones. Esto amplía armas que el
runtime puede cargar, pero no implementa todavía objetos del mundo, botín ni
recogida/equipamiento.

## Iteración 066: RNG de agua enlazado al mapa ACT_ONE

`floor_layout.py` ahora ejecuta los 60 ticks de agua con el MT19937 del
generador y consume los draws de burbujas. El tamaño 1024 del pool y su lista
inicial libre provienen del constructor `Level`/`ParticlePool`; `sealOffSecretRooms`
no asigna partículas antes del agua. `simulateAllWater` no avanza ni recicla
partículas durante esos ticks. Por tanto, el estado y los draws quedan
reconstruidos hasta `water_particle_warmup`, con la salvedad de que no existe
comparación de snapshots por semilla contra una ejecución del juego PC.

La salida privada `_local/generation066-final/map-export` se generó desde la copia
local de `rooms.json` y los recursos; seed 12345, piso 0 conserva entrada
`[30,4]`, salida `[36,28]`, una ruta de tile de 58 pasos y ocho enemigos todavía
provisionales. En esta seed el agua visitó 3975 celdas activas y consumió 3979
draws enteros más dos draws flotantes para dos burbujas; el pool terminó con
1022 slots disponibles. El barrido de 20 seeds por cada uno de los tres pisos
verifica que el estado registrado coincide con los draws consumidos y que los
pisos siguen dentro de capacidad con spawn y ruta topológica.

Después del agua, el exportador cuenta los `DOODAD_BOOK_PEDESTAL` sin objeto y
consume el draw MT conocido del constructor `Item` que inicializa `randIndex`.
El ledger es parcial: con varios pedestales, cada constructor se intercala con
su bucle de skills; por eso no se presenta como estado final de entidades.

El flujo se detiene antes de `addStartAndGoal`: `createRoomEntities` se ejecuta
primero y un `DOODAD_BOOK_PEDESTAL` sin libro llama a
`generateNewRandomSpellBook` (`0x00643c90`). El libro de tipo 216 evita en
`generateNewRandomItemOfType` (`0x006435d0`) las ramas RNG de maldición/afijos,
pero luego hay cinco iteraciones que condicionan `uniform_int` según el Skill
actual y el piso. `Item::setSkill` (`0x00623680`) puede consumir otro draw si la
skill elegida difiere de `randIndex & SUMMON_MONSTER` (enum 15); skills fuera de
0–15 salen por la rama inválida. Así, el coste no es un fijo de cinco draws.
Falta reconstruir la condición del campo `Skill+0x70c`, la distribución exacta
de `uniform_int`, el estado `randIndex`/`Skill` y la ubicación final de entidades
G/H. Hasta entonces, no se debe usar el estado post-agua para afirmar que los
shuffles de endpoints sean seed-exact. Enemigos exactos, gameplay físico, VPK
y replay de Vita3K siguen pendientes; el piso no se declara completo.

## Iteración 067: draw conocido del constructor de libros

El ledger parcial de `floor_layout.py` cuenta los doodads de tipo
`DOODAD_BOOK_PEDESTAL` sin `heldItem` y consume un draw `game_int()` por la
construcción del `Item` que contiene el spellbook. `Item::Item` recuperado de
Ghidra (`0x00616000`) toma `uniform_int<int>` para fijar `randIndex`; el
`uniform_int` del ejecutable usa un rango de 31 bits y un word MT por llamada.
El export `_local/generation067-final2/map-export` tiene un pedestal para seed
12345/floor0, y verifica que el contador de draw del constructor coincide con
ese número.

Este ledger no finge el estado final: el Item después corre el bucle de cinco
skills con guardas que consultan el Skill actual y piso. Cada `setSkill` puede
consumir otra llamada `uniform_int` si el skill nuevo difiere de los bits bajos
de `randIndex`; además, esos draws se intercalan inmediatamente por pedestal.
Por eso se conserva `full_room_entity_rng_reconstructed=false` y los endpoints
siguen sin ser seed-exact. El barrido de 60 combinaciones (20 seeds × 3 pisos)
verifica las cuentas del agua y el ledger parcial del constructor, además de
spawn y ruta topológica.

## Iteración 068: selección de skills de los libros

El offset consultado por `generateNewRandomSpellBook` se resolvió con
`fields.tsv`: `Skill.spellTier` está en decimal 1804 (`0x70c`). El constructor
Ghidra (`0x0078b150`) asigna los tiers por SkillType; los primeros 16 IDs son
precisamente los que permite la máscara `0x8000000f` del generador. El umbral
del bucle de cinco intentos es `1 + floorIndex // 3`. Tras cada selección,
`Item::setSkill` toma una tirada adicional si el SkillType difiere de
`randIndex & 0xf`, y actualiza ese índice con el mismo valor aleatorio. Una vez
que el tier actual no supera el umbral, el Skill no cambia y las iteraciones
restantes tampoco toman draws.

`floor_layout.py` ahora reproduce constructor, selector y ajuste de índice por
pedestal, en orden de creación. Registra IDs seleccionados, draws y tier final.
La suite completa pasa 137 pruebas y el barrido usa 20 seeds × 3 pisos. Esto
solo cierra el RNG de los libros: otros objetos y enemigos pueden consumir RNG,
y el mapa de bloqueos G/H y el shuffle de endpoints siguen pendientes. Por ello
`full_room_entity_rng_reconstructed` permanece falso.

## Iteración 069: shuffles y orden de candidatos de endpoints

Los listings `0065a9a0.c`, `0065aca0.c`, `006960c0.c` y `00689c60.c` muestran
que la entrada mezcla el grupo preferido o, si está vacío, el próximo al agua;
la meta mezcla primero el vector alcanzable y vuelve al selector de entrada
cuando hay menos de ocho candidatos. Se conectó el `std::shuffle` MSVC al MT
común. El comparador de meta se expresa por vecinos horizontales abiertos,
soporte sólido, tipo no-plataforma y Y descendente.

El barrido de 60 combinaciones pasa por la rama de fallback en 11 casos. Seed
12345/floor0 exporta entrada `[34,6]`, salida `[38,29]` y camino topológico de
55 pasos en `_local/generation071/map-export`. La suite Python pasa 139 pruebas.
El mapa queda provisional: siguen sin modelar otros hooks RNG de entidades y
la lista de blockers G/H, y el orden de empates del `std::sort` no se contrastó
con MSVC. El route probe con la física Vita no pudo compilarse porque no hay
compilador host y WSL denegó el acceso. No hubo cambio C, build VitaSDK ni VPK;
el último paquete continúa siendo 00.65. Vita3K y gameplay permanecen pendientes.

## Iteración 070: mapa de blockers de entrada y meta

`LevelGenerator::addStartAndGoal` (`0x00688e30`) registra en un set los tiles
de entidades tipo `'G'` y `'H'`, luego omite esos puntos al seleccionar
entrada y meta. Para las ubicaciones de habitación, el generador redondea los
pixeles como `Room::createEntity` y cuantiza por tiles; también reproduce el
Boulder enlazado que `BoulderTrap::setLevel` coloca encima de la trampa cuando
la columna tiene soporte sólido.

El informe deja el mapa de blockers y los candidatos rechazados. Suite Python:
140/140; el barrido de 60 combinaciones evita elegir posiciones bloqueadas y
descarta al menos un candidato en 13 casos. Seed 12345/floor0 conserva entrada
`[34,6]` y salida `[38,29]` (VWI2 SHA-256
`b410f43eb5b429c339643715f28d1ebb3f1d1f60803aaf77803d90bb6d9edcbe`). No se
compiló VPK: runtime sin cambios y último paquete 00.65. Otros hooks RNG y el
desempate MSVC de `std::sort` no se han reconstruido; el mapa no es seed-exact.
El route probe C sigue pendiente por falta de compilador host y el `E_ACCESSDENIED`
actual de WSL.
