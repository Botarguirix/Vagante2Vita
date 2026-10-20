# Iteración 133: stats y combate base del Goblin King

Se recuperaron listings de instrucciones de Ghidra desde una copia local de la
base del proyecto, conservando intacta la instancia original. La tabla PDB
identifica `Monster.healthScaleFactor@820`, `damageScaleFactor@824` y
`newGamePlus@833`; `Entity.hp/maxHp` están en `268/272`, y `Unit.damage` y
`moveSpeed` en `536/544`. `Monster::Monster` inicializa ambos factores a 1 y
NG+ a false. El listing de `BossGoblinKing::initializeStats` escribe HP y
maxHP como factor de salud ×70, daño como factor de daño ×8 y velocidad 1.5.
La rama NG+ usa ×210 HP, ×16 daño y velocidad 1.8. `BossGoblinKing::hurt`
delega en `Unit::hurt` y, ante resultado letal, crea recompensa y elimina el
jefe activo.

El runtime VWE3 ya inicializa el Goblin King normal con 70 HP. Melee y flechas
pueden reducir su HP; cada serial de golpe melee cuenta una vez. El dash aplica
8 de daño en su primera intersección con el jugador durante cada carga. Se
añadió el gate de impacto de carga y pruebas host para HP, melee, flecha y daño
de carga.

Validación: las 12 pruebas nativas host compiladas con Clang/UCRT y las 194
pruebas Python pasan. Se
compiló el VPK 01.31 desde WSL invocando el VitaSDK disponible en Windows; su
SHA-256 es `E4A80284004C93EC9D0DF084E75963F84EC43915AE8A28C9410451B6FDFC7668`.
Las tres rutas físicas de campaña se regeneraron contra las fuentes actuales y
completan los pisos 0/1/2 en 438/890/546 ticks; la copia de entrega con los
proofs nuevos valida sus 142 archivos. Se verificó el ejecutable por
`ProductVersion`: las corridas anteriores por ID y `content-path`, además de
la prueba de ventana sin argumentos, usaron Vita3K 0.2.1.4183. Una repetición
visible del VPK 01.31 con 0.2.1.4190 también termina tras 180 s sin sesión
nativa ni ventana principal; el filesystem contiene los archivos VPK, pero el
emulador no los procesa. `vita3k.log` y los logs de depuración quedan vacíos.
`--version` no respondió en 10 s en 4183 (también con
`QT_QPA_PLATFORM=offscreen`) ni en 4190, sin salida ni error. El resultado de
la corrida 4190 está en
`_local/vita3k-act-one-131-4190/runs/iteration131/run-000001/`.
La CLI oficial distingue `content-path` (instalar y ejecutar un VPK) de
`--installed-path/-r` (ejecutar una app instalada), y ambos caminos se probaron.
Por ello, la evidencia apunta al arranque del proceso de Vita3K en este entorno,
antes de que procese el VPK; el runtime del juego sigue sin validación en
emulador. El exporter ACT_ONE ahora añade un Rey Goblin provisional al piso final, en
un ancla geométricamente validada; con seed 4 queda en el tile [21,20]. La
selección determinista no reproduce addBoss ni el RNG nativo. El paquete de
entrega recalcula sus hashes y valida 142 archivos. El runtime aún no implementa
recompensa/muerte, persistencia del ciclo de jefes ni transición tras derrotarlo;
el actor es un encuentro de prueba integrado, no un jefe de campaña completo.
La nueva build 01.32 incorpora el log que identifica este modelo de combate
provisional. La prueba de emulador sigue pendiente por el bloqueo de arranque
documentado arriba.

Diagnóstico adicional del host: el runtime aislado contiene el mismo conjunto
de archivos de ejecución que el origen (incluidos Qt, plugins, share, shaders
y traducciones). Abrir Vita3K sin argumentos y sin redirigir sus flujos tampoco
crea ventana principal (`MainWindowHandle=0`). El aviso visto por el usuario al probar 01.09 se
cerró antes de poder registrar su texto, y no se confunde con un fallo nativo
del juego porque no apareció ningún `session-*.log`.

Referencia de la CLI usada para separar ambas rutas de lanzamiento:
[Vita3K `config.cpp`](https://github.com/Vita3K/Vita3K/blob/master/vita3k/config/src/config.cpp).

Artefactos reproducibles de Ghidra: `_local/analysis132/initializeStats.listing.txt`,
`boss-hurt.listing.txt`, `unit-hurt.listing.txt` y
`boss-collision.listing.txt`. El proyecto usado es la copia aislada
`_local/analysis132/ghidra-copy`; los binarios y datos comerciales no se
subieron a servicios externos.


## Revalidación 01.32 en Vita3K aislado

El preflight detectó y corrigió una omisión del auditor: VWE3 ahora distingue
el actor enemigo (tipo 24) del Goblin King de prueba (tipo 26, subtipo 49), y
valida sus coordenadas y atlases en `bosses/goblin_king/`. El perfil local
`_local/vita3k-act-one-132-4190/` valida los tres pisos y 93 imágenes; el
paquete contiene 13 roles de sprite del Rey Goblin y 23 atlas de referencia.
Se regeneraron los proofs y trazas de los tres pisos contra la huella actual del
simulador; sus rutas completan en 438/890/546 ticks y la secuencia de campaña
se vuelve a generar con 250 pasos y 2,206 ticks nominales.

La prueba de campaña con Vita3K 0.2.1.4190 expiró tras 90 s al abrir el título
instalado (OpenGL/Qt). Una segunda prueba por VPK directo/console expiró tras
45 s. Ambas terminaron antes de `session-*.log`; no hubo ventana principal y
`vita3k.log`, stdout y stderr quedaron vacíos. Sus resultados están en
`_local/vita3k-act-one-132-4190/runs/iteration132/run-000003/` y
`run-000004/`. Se probó además Vita3K 0.2.1.4190 sin argumentos en el runtime de prueba y
en su perfil fuente; ambas instancias quedaron con proceso vivo, sin ventana
principal y sin log. Repetir el arranque sin argumentos con `PATH` reducido a
la carpeta runtime y directorios de Windows produjo el mismo resultado. Por
tanto, no se atribuye el fallo al juego ni se afirma que ACT_ONE esté validado
en emulador; continúa sin verificarse la integración en Vita3K.


## Iteración 01.33: derrota provisional y cofre de recompensa

La inspección adicional de Ghidra confirma que el camino letal de
`BossGoblinKing::hurt` pide a `Level::spawnBossReward` crear una recompensa y
después elimina el jefe activo. El listing muestra también que
`ItemGenerator::generateNewBossRewardItem` toma en cuenta `floorIndex + 3` y
genera un objeto; las tablas disponibles no permiten identificar con certeza
el tipo exacto ni reproducir su RNG nativo. El runtime ahora cuenta jefes
cargados/derrotados y, al derrotar al actor de prueba con melee o flecha, crea
una sola caja en su posición. Su contenido usa provisionalmente el pool VLT1
anidado 23 al nivel `piso + 3`; el log declara `native_parity=0`. Esto permite
probar el ciclo de combate/recompensa sin presentarlo como reproducción exacta
del juego original.

Validación local: `python -m unittest discover -s tests -v` pasa 197/197;
`tools/run_host_tests_windows.ps1` pasa 12/12, incluidas las regresiones de
melee, flecha y cofre de jefe. La campaña se prepara con 250 pasos, 2,206 ticks
nominales y tres rutas ligadas a las fuentes actuales. El paquete de campaña
valida sus 142 archivos. VPK 01.33:
`_local/build-act-one-133/Vagante2Vita-iteration133.vpk`, SHA-256
`486B4CCC9AD2DC0742216FC7E965E5A36A7C91BB1313D7C539E03F350F6AEB46`.

La repetición automatizada visible contra Vita3K 0.2.1.4190 agotó el límite de
30 s antes de crear `session-*.log`; el reporte indica
`failure_stage=before_native_session`, y stdout, stderr y `vita3k.log` están
vacíos. El proceso principal siguió vivo pero sin ventana (`MainWindowHandle=0`);
hay además un proceso Vita3K sin ruta ejecutable identificable, que se dejó
intacto por no poder confirmar si pertenece a otro proyecto. Por tanto, el VPK
está construido y validado en host, pero esta iteración todavía no cuenta como
ejecutada en Vita3K. Reporte: `_local/vita3k-act-one-133-4190/runs/iteration133/run-000001/`.
