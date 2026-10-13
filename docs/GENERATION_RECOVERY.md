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
