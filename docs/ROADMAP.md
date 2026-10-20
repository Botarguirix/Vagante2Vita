# Estado de reconstrucción (18 de octubre de 2026)

## Iteración 01.99: render 640×360 y rutas de campaña con clase fija

El modo lógico de 640×360 se mantiene como predeterminado y se escala a
960×540 dentro del framebuffer Vita de 960×544. En menú, L permite bajar a
320×180. El build 01.99 se recompiló y su VPK contiene las siete entradas
permitidas, con identidad VAGV00006 / 01.99. La batería completa pasa: 13
suites C host, 226 pruebas Python y 70 subpruebas.

Las pruebas de ruta ahora registran la clase del personaje y el empaquetado
rechaza conjuntos de pisos con clases mezcladas. El controlador host ensaya
saltos evasivos cerca de enemigos durante el Dragon del piso 0 y completa los
tres recorridos Rogue aislados. La cadena con salud persistente mejora de
10/80 a 20/80 HP al terminar el primer piso, pero muere en el piso 1 en tick
1015 por contacto del Worm/Bat. La ruta Rogue previa moría en tick 918. El
probador registra clase, HP máximo/actual y flechas iniciales, admite estados
cargados y rechaza una traza cuyo estado no coincide con el transferido por la
campaña. Se detectó que la traza aislada del piso 1 comenzaba con 80/80 HP y 30
flechas, frente a 20/80 y 24 reales; al planear desde esos valores, el
controlador actual aún muere en tick 1015. El ajuste evasivo solo cambia la
generación de replay de prueba para el Dragon del piso 0; el controlador humano
del juego no cambia. Las rutas separadas no prueban ACT_ONE completo: la
siguiente prioridad es mejorar la navegación/evasión del piso 1 y demostrar
dos transiciones en una sola cadena viva.

El VPK se compiló, pero Vita3K 0.2.1.4194 en la copia completa aislada agotó
45 segundos antes de crear la sesión nativa. El diagnóstico posterior, incluso
sin lanzar un título, reprodujo `0xC0000005` en SDL y Qt. La excepción parte de
`ucrtbase.dll+0x25F31`, con retorno a `Vita3K.exe+0x683965`; la copia no incluye
PDB para resolver una función. Por eso el VPK y la transición aún no se han
validado dentro del emulador. Las trazas están en
`_local/vita3k-c197-fullcopy-test/diagnostics/no-title-startup-20261019/`.

Una táctica de salto cada 12 ticks cerca de enemigos no mejoró la cadena: el
Rogue murió en tick 1022 y apenas dañó al Worm (4.5 HP). El salto continuo
generó un recorrido sin terminación útil y se descartó. La línea base sigue
muriendo en tick 1013 tras varios contactos de Worm/Bat. La siguiente mejora
debe cambiar la aproximación al saliente del Worm y permitir disparar al
mantenerse fuera del alcance, no aumentar saltos sin objetivo. La traza de
posiciones del estado 20 HP/24 flechas está en
`_local/worm-carry20-baseline-positions.csv`. Véase
[Demo ACT_ONE 01.99](DEMO_ACT_ONE_199.md).

## Actualización 166x

La ruta host nativa completa los tres pisos del bundle privado: derrota al
Dragon, Worm y Goblin King de prueba, mantiene al jugador vivo y toca cada meta
real. El paquete `_local/campaign-act-one-complete-166x` valida y contiene
proofs v3 con trazas del controlador. Esto verifica la física/ruta de la
reconstrucción, pero los jefes y su selección siguen provisionales y no hay
paridad exacta de generación ni prueba de transición en Vita3K.

Las 13 suites C host y las pruebas del generador de replay y del checker
Vita3K pasan. El VPK 01.65 y los 144 recursos pasaron preflight. Vita3K
0.2.1.4194 de `C:\Vita3k`, incluso con perfil completo aislado, vence el
timeout antes de crear una sesión nativa; por ello no afirmamos que haya
validado la puerta de entrada o la campaña. El detalle está en
[WORM_BUILD_DIAGNOSTIC.md](WORM_BUILD_DIAGNOSTIC.md) y el estado de jefes en
[BOSS_DECOMP_LEDGER.md](BOSS_DECOMP_LEDGER.md).

El reintento directo con `C:\Vita3k\Vita3K.exe` y `--config-location` al perfil
aislado también expiró a los 20 segundos sin stdout/stderr ni log fresco. No se
pudo demostrar que esa opción mantuviera `VitaFS` aislado; esta ejecución no
cuenta como smoke válido. El detalle está en la última sección de
[WORM_BUILD_DIAGNOSTIC.md](WORM_BUILD_DIAGNOSTIC.md).

La revisión de `generateLevel` corrigió además el diagnóstico RNG: endpoints se
seleccionan antes de `addRandomEnemies`, así que filtros de enemigos/jefe solo
afectan draws posteriores. Los reportes ahora separan ambas fronteras y los
tests comprueban que el trace enemigo comienza después del intervalo RNG de
endpoints. Otro test fija el orden x-major y la partición seco/agua recuperados
de Ghidra. La suite completa pasa: 211 pruebas y 70 subtests.

La iteración 01.63 consolida la transición que corrige el reinicio observado
con el VPK antiguo 01.55. Desde el spawn, la traza alcanza la cueva manteniendo
derecha 38 ticks; el VPK y los 142 datos de campaña pasan sus verificadores.
Vita3K no creó sesión nativa en copias aisladas de 0.2.1.4170, 0.2.1.4183 y
0.2.1.4194. El binario 0.2.1.4194 de `C:\Vita3k` también cae al iniciar con
`--skip-title`, antes de cargar el VPK. El harness ahora limpia la instalación
y los datos propios del título dentro del sandbox y registra hashes de VPK,
replay y recursos; la corrida confirmó ese reporte, pero el frontend volvió a
fallar antes de una sesión. La replay histórica de 00.25 se regeneró idéntica,
pero tampoco pudo ejecutarse en el clon local. Falta recuperar la
configuración exacta del PASS antiguo y congelar perfil, firmware y hashes de
entrada. Véase [ITERATION163.md](ITERATION163.md).

La iteración 01.61 hace explícito en VTS1 el modo de empezar en el intro y
añade un smoke que reproduce la entrada por la cueva y completa el primer piso
ACT_ONE. La ruta intro alcanza la meta en 38 ticks; la replay completa tiene
77 segmentos / 674 ticks y enlaza trazas `player_step` del intro y del piso 0.
Pasan 13 grupos C y 206 pruebas Python; el paquete de 142 archivos valida y el
VPK 01.61 compila. Vita3K 0.2.1.4190 y 0.2.1.4170 (Qt/OpenGL con DLLs
presentes) en clones aislados terminan con `0xC0000005` antes de la sesión
nativa; repetir 4170 bajo una ruta corta tuvo el mismo resultado. El smoke no
llegó al juego. Véase [ITERATION161.md](ITERATION161.md).

La iteración 01.60 prioriza el mapa introductorio con la cueva en partidas
normales cuando sus datos existen; ACT_ONE queda como respaldo y sigue siendo
la entrada directa de las replays automatizadas. Se confirmó con el probe
nativo de física que el spawn del intro alcanza la meta de la cueva en 38
ticks, sin salto ni colisión atascada. Pasan 13 grupos C y 204 pruebas Python;
VPK 01.60 compilado y su identidad/Self verificados. La transición todavía no
se validó en Vita3K, que volvió a quedarse antes de crear sesión para 01.59.
Véase [ITERATION160.md](ITERATION160.md).

La iteración 01.59 elimina la asignación de Círculo a reiniciar y hace que la
salida del mundo inicial de respaldo cargue el piso 0 de ACT_ONE cuando los
datos están instalados; si faltan, deja un error visible. La ayuda de juego
indica la secuencia inicial derecha y abajo+X. Pasan 13 grupos C, 204 pruebas
Python y tres rutas host. Vita3K 0.2.1.4170 y 0.2.1.4190 no iniciaron una
sesión nativa en sus clones privados; puerta y transición siguen sin
confirmación visual. Véase [ITERATION159.md](ITERATION159.md).

La iteración 01.56 mueve el spawn desde el centro del marcador de entrada tipo
10 hacia el lado lateral despejado y apoyado, orientado al recorrido. Pasan 13
grupos C host y 204 pruebas Python; los tres recorridos host llegan a su
objetivo, incluido Goblin King. VPK compilado. Se intentó correr la campaña en
Vita3K 0.2.1.4190 sobre clon completo aislado: agotó 120 s sin crear sesión
nativa, por lo que no alcanzó a ejecutar el juego. La puerta sigue sin prueba
visual en emulador. Véase [ITERATION156.md](ITERATION156.md).

La iteración 01.55 corrige la colisión horizontal contra paredes: el cálculo
anterior podía elegir el tile vacío adyacente y desplazar al jugador varios
píxeles hacia atrás al contacto. La regresión con pared en x=320 y jugador en
x=311.75 confirma que ahora se alinea con la cara sólida. Pasan 13 grupos C
y se compiló el VPK 01.55. La suite Python más reciente pasó 204/204. Vita3K
0.2.1.4190 agotó 120 s sin ventana ni sesión nativa con un clon completo de
2,162 archivos; 0.2.1.4183 terminó con `0xC0000005`. Ninguno ejecutó el juego.
ACT_ONE sigue incompleto. Véase [ITERATION155.md](ITERATION155.md).

La iteración 01.54 añadió una línea de visión por tiles al gate provisional de
ataque del Dragon. Pasan 13 grupos C host, 204 pruebas Python y sintaxis ARM;
las rutas host de tres pisos pasan y el replay prepara 607 pasos. El VPK 01.54
se instaló en una copia completa de Vita3K 0.2.1.4183, pero el emulador cayó con
`0xC0000005` antes de la sesión nativa. Véase [ITERATION154.md](ITERATION154.md).

La iteración 01.53 usó los exports y enums de Ghidra para recuperar la cola de
acciones del Dragon (15 acciones, ramas de disparo, vuelo y buceo). El runtime
aún conserva su persecución y disparo provisionales; no se trasladó la IA hasta
confirmar los campos superpuestos y el estado del MT19937. No cambió el VPK.
Véase [ITERATION153.md](ITERATION153.md) y la fila Dragon de
[BOSS_DECOMP_LEDGER.md](BOSS_DECOMP_LEDGER.md).

La iteración 01.52 repitió el VPK 01.50 con un perfil completo aislado (2,162
archivos verificados): Vita3K 0.2.1.4183 termina con `0xC0000005` antes de
crear sesión nativa, aun con Qt/OpenGL visibles. El replay de tres pisos se
prepara, pero no se ejecuta en el emulador. Logs viejos heredados del perfil se
descartaron como evidencia. Pasan 204 pruebas Python y 13 grupos C host; la
causa del crash y la campaña jugable siguen pendientes. Véase
[ITERATION152.md](ITERATION152.md).

La iteración 01.50 incorpora un Dragon provisional tipo 50 como encuentro de
prueba en el primer piso de ACT_ONE y bloquea la salida hasta derrotarlo. Su
ancla evita la entrada estrecha al objetivo. El probe host reconoce ambos jefes
soportados y carga el arco, pero no ha conseguido demostrar esta pelea física:
las flechas chocan con la plataforma y el jugador puede morir. La copia completa
de Vita3K contiene 2,126 archivos verificados y el VPK 01.50 está instalado, pero
Vita3K no creó una sesión nativa. Pasan 204 pruebas Python y 13 pruebas C host;
la decompilación, la victoria del Dragon y el smoke en Vita3K siguen pendientes.
Véase [ITERATION150.md](ITERATION150.md).

La iteración 01.49 integra el ciclo provisional de fuego del Dragon: el atlas
`launching_fireballs` de cinco cuadros se reproduce y genera un proyectil con
velocidad normal 1.2, colisión con terreno y daño único al jugador. Véase
[ITERATION149.md](ITERATION149.md).

La iteración 01.48 corrige el enfriamiento del contacto provisional del Dragon:
ahora se reduce cada tick de combate y el atlas se voltea según el lado al que
mira. El host test cubre contacto, cooldown y segundo impacto. Esto mejora la
pelea provisional, pero no recupera ataques de proyectil ni IA nativa.
[ITERATION148.md](ITERATION148.md).

La iteración 01.47 mueve al Dragon con persecución aérea y colisión contra
tiles; su contacto provisional inflige el daño base recuperado (5) con
enfriamiento. Host tests cubren desplazamiento, hitbox y daño. Se quitó Worm
del combate genérico: el Ghidra export confirma que necesita 20 crawlers
enlazados, aún ausentes. El Dragon no tiene proyectiles ni paridad de IA y el
gate de campaña sigue requiriendo su roster. Véase
[ITERATION147.md](ITERATION147.md).

La iteración 01.46 conserva un registro de derrotas de los tres tipos de jefe
de ACT_ONE a través de cambios de piso y la salida final exige la máscara
completa. Los logs y el checker de Vita3K ahora rechazan una finalización
basada solo en el Goblin King. Esto hace visible una limitación real: el Dragon
no tiene IA de jefe, Worm no está implementado y los paquetes actuales aún no
permiten derrotar los tres tipos; por eso la campaña completa todavía no es
alcanzable. Los tres IDs proceden del selector nativo de ACT_ONE en Ghidra;
la distribución exacta de encuentros por piso sigue pendiente de validar en
una sesión nativa. Véase [ITERATION146.md](ITERATION146.md).

La iteración 01.45 corrige el hitbox cuerpo a cuerpo del Dragon: usa el cuerpo
13×13 confirmado en el constructor, con una prueba que cubre su franja inferior.
Pasan 13 pruebas C, 202 Python y 70 subtests; los proofs físicos de los tres
pisos se regeneraron y pasan. El VPK 01.45 compila. El smoke completo valida
2,126 archivos, el paquete de 142 archivos y prepara el replay, pero Vita3K
vuelve a expirar antes de sesión nativa. El Dragon aún carece de IA y Worm de
sus 20 segmentos; ACT_ONE permanece provisional. Véase
[ITERATION145.md](ITERATION145.md).

La iteración de runtime 01.44 extrae y prueba la decisión de salida de ACT_ONE:
avance en los pisos 0/1, gate de jefe vivo en el piso final, cierre del acto y
salida intro. Los proofs host se regeneraron con su nueva huella y las 13
pruebas C + 202 Python pasan. VPK 01.44 compila con VitaSDK; el runner verifica
SHA-256 de los 2,126 archivos de la instancia aislada. SDL, Qt y consola/VPK
directo agotaron el plazo antes de crear sesión nativa, con logs vacíos. No
equivale a validación del juego. Véase [ITERATION144.md](ITERATION144.md).

La iteración 136 añadió una traza física de los tres pisos que derrota al
Goblin King provisional del piso 2 y alcanza la salida: 2,161 ticks en la
pelea final, con 41 HP restantes. El runtime bloquea la salida si el jefe
sigue vivo. Las rutas de los pisos 0/1/2 quedaron ligadas a las fuentes
actuales y el replay consume las tres (`player_step:0/1/2`). El ancla usa la
geometría candidata conectada `(28,21)`; generación exacta y paridad nativa
siguen sin verificarse. El VPK 01.36 compila; Vita3K 0.2.1.4183 y 0.2.1.4190
caen con `0xC0000005` antes del log nativo aun usando perfiles completos con
`pref-path` aislado. Véase [ITERATION136.md](ITERATION136.md).

La corrección del exportador alinea el cuerpo del Goblin King con el centro del
tile (`[456,344]`); usar el origen del tile hacía que la ruta física se
atascara tras el combate. Los proofs y las trazas se regeneraron: los tres
pisos pasan y el jefe cae antes de la salida en el probe host. No cambió el
VPK, porque este ajuste pertenece al generador de paquetes. Vita3K sigue sin
validar el runtime debido al cierre previo a la sesión.

La iteración 143 añadió un runner host Windows reproducible con Clang/UCRT,
proofs verificados contra la misma huella del empacador y paquete actualizado
de 142 archivos. El replay de tres pisos se reconstruye desde esas trazas (494
pasos, 3,821 ticks). La suite Python (201) y la suite C host (12) pasan. Esto
mejora la prueba automatizada local; no reemplaza el replay pendiente dentro de
Vita3K ni la paridad con el generador nativo. Véase
[ITERATION143.md](ITERATION143.md).

El build privado 01.36 se recompiló desde el código C actual con VitaSDK y se
instaló en un clon íntegro aislado. SDL/OpenGL, consola con instalación VPK por
CLI y un lanzamiento visible agotaron sus plazos antes del primer log nativo;
controles sin título tampoco mostraron ventana ni logs. El ejecutable anterior
local reprodujo el estado. La evidencia señala la inicialización de Vita3K en
este entorno, pero no identifica su causa ni valida el juego. El nuevo paquete
mantiene `campaign_complete=false`; siguen faltando la ejecución real en
emulador, Dragon/Worm integrados, RNG exacto, recompensa nativa y paridad de
generación. Detalle y hashes en [ITERATION143.md](ITERATION143.md).

La iteración 133 recuperó por listing y PDB las fórmulas normales del Goblin
King: 70 HP y 8 de daño con los factores iniciales de Monster en 1. El actor
VWE3 ahora recibe melee y flechas, y daña una vez por carga; pasan 12 pruebas
host y pasan las 194 pruebas Python. El VPK 01.31 compila y las tres rutas
físicas pasan. Vita3K 0.2.1.4183 y 0.2.1.4190 siguen expirando antes de crear
sesión nativa; las pruebas de CLI tampoco responden a `--version` en 10 s. No
se ha validado la ejecución del juego en emulador; véase el detalle
en [ITERATION133.md](ITERATION133.md).
El exporter de campaña sigue sin instanciar jefes, por lo que no se declara un
encuentro ACT_ONE integrado. Véase [ITERATION133.md](ITERATION133.md).

La revisión de Decompedia confirma que sirve como índice y guía metodológica,
no como código que podamos incorporar directamente. Ghidra rechazó la base
original por diferencia de propietario; la iteración 133 resolvió el análisis
desde una copia local aislada y guardó los listings de stats/daño. Véanse
[ITERATION132.md](ITERATION132.md) y [ITERATION133.md](ITERATION133.md).

La auditoría de la iteración 131 evitó tratar el antiguo `hp=1` de los paquetes
VWE3 como salud real. La iteración 133 recuperó la fórmula nativa normal e
inicializa el actor con 70 HP; el exporter y el flujo de derrota aún impiden
considerarlo una pelea integrada. Véase [ITERATION131.md](ITERATION131.md) y
el [ledger](BOSS_DECOMP_LEDGER.md).

La iteración 130 implementa el movimiento y atlas de carga del Goblin King
como comportamiento provisional. El registro VWE3 lo ejecuta si se provee,
pero ACT_ONE todavía no genera jefes ni implementa daño/derrota/transición. Las
rutas físicas seed 1/49/4 pasan en 438/890/546 ticks, 12 pruebas C y 194 Python
pasan, y el VPK 01.30 compila. Vita3K 0.2.1.4190 sigue expirando antes de abrir
sesión nativa. Véase [ITERATION130.md](ITERATION130.md) y el
[ledger de jefes](BOSS_DECOMP_LEDGER.md).

La iteración 129 conecta el golpe del Thief con su spritesheet de ataque y la
128 integra su ataque provisional de corto alcance (daño 1–2, cooldown 60
ticks, caja aproximada). Véanse [ITERATION129.md](ITERATION129.md) y
[ITERATION128.md](ITERATION128.md).

Al cierre de la iteración 129, ACT_ONE todavía no estaba completo ni validado en emulador: la campaña host
recorre tres paquetes de piso, pero el exportador no genera jefes y no existe
combate/derrota/continuación de jefes. `BossGoblinKing::chargeAttack` aporta
ahora valores confirmados de telegraph/impulso (35 o 75 ticks y ±9.3 px/tick),
con colisión y daño aún por recuperar; el ledger conserva esta evidencia sin
presentarla como una IA implementada. Véase
[BOSS_DECOMP_LEDGER.md](BOSS_DECOMP_LEDGER.md).

La revisión de `decomp.wiki` y la wiki técnica de decompilación quedó aplicada
al flujo de análisis: evidencia por función/CFG/tipos y validación por etapas,
sin confundir reconstrucción funcional con matching. Detalle en
[referencias públicas](REFERENCE_REPOSITORIES.md#hallazgos-de-las-wikis-y-aplicación-al-análisis-de-vagante).

La iteración 125 corrige la rama Thief: el gate inicial exacto es `roll % 5 ==
0` y `floorIndex > 0`, con presupuesto local máximo de un ladrón. En 100 seeds
por piso produjo 0/100, 23/100 y 21/100 gates habilitados (pisos 0/1/2). Esto
solo valida el gate; todavía no se simula la posición del spawn. Véase
[iteración 125](ITERATION125.md).

La iteración 122 organiza el análisis por función en un ledger de evidencia
para los jefes ACT_ONE y prepara los 23 atlas originales de Goblin King,
Dragon y Worm en el bundle privado, con validación de manifiesto y PNG. Aclara que el proyecto
reconstruye comportamiento para un port y no realiza matching decompilation;
el reporte de sala/anclas aún no constituye un spawn, faltan el RNG exacto, los
bloqueos dinámicos y el runtime de los tres jefes. Véase
[BOSS_DECOMP_LEDGER.md](BOSS_DECOMP_LEDGER.md) y
[ITERATION122.md](ITERATION122.md).

El seguimiento de runtime acepta un registro de prueba Goblin King y dibuja su
atlas estático, con pruebas host para el contrato VWE3. El exportador todavía
no crea encuentros de jefe y no hay IA, daño, derrota ni transición. El smoke
01.12 de tres pisos validó el paquete, pero Vita3K agotó 90 s antes del log
nativo; por tanto no cuenta como prueba de juego. Véase [iteración 123](ITERATION123.md).

La iteración 124 corrige la lectura del filtro `E/G/K/T`: `T` es Thief
(`EntityType` 84), con cuerpo 7×12, no Shrine. `addRandomEnemies` puede crear
ladrones con un gate inicial exacto: entero `%5 == 0` y piso posterior al
primero, con presupuesto local máximo de uno. El enum `4` pertenece a
`ActType`, no a una sala. La traza registra el gate sin simular el spawn porque
faltan la asociación de la entrada, el orden de candidatos y el punto de
aparición. Véase
[iteración 125](ITERATION125.md).

La iteración 121 ejecutó un control Vita3K 0.2.1.4190 sin título desde el
filesystem aislado: `--version` termina con código 0, pero el frontend normal y
`--load-app-list` quedan sin ventana, sin log y sin cargar Qt. Esto ubica el
bloqueo antes del título, aunque aún no explica la falla de inicialización.
Véase [ITERATION121.md](ITERATION121.md).

La iteración 120 separa los errores WER históricos de `G:\Vita3k` (0.2.1.4182
y 4183, `0xc0000005`, 14 de octubre) de los tres timeouts en las instancias
privadas de 1.12: no hay un evento WER nuevo asociado a esos intentos, y la
ventana de error detectada no se pudo atribuir. Por eso la causa de no alcanzar
sesión nativa sigue sin resolver; no se alteró la instancia compartida.
Véase [ITERATION120.md](ITERATION120.md). El build y los resultados del smoke
están en [ITERATION119.md](ITERATION119.md).

La iteración 119 produjo y verificó estructuralmente el VPK 01.12 con VitaSDK
Windows 15.2.0. El preflight de campaña confirma tres pisos, pero tres
ejecuciones aisladas (Vita3K reciente OpenGL/SDL, 0.2.1.4170 OpenGL/SDL y
0.2.1.4170 consola) terminaron antes de crear sesión nativa, todas con logs
vacíos. No hay todavía validación de gameplay. WSL permanece bloqueado por
`E_ACCESSDENIED`; se usó el toolchain Windows disponible. Véase
[ITERATION119.md](ITERATION119.md).

La iteración 118 incorpora al trace provisional la rama de enemigos acuáticos
de `addRandomEnemies`: masa de fluido, selección Piranha/Minnow y pesos nativos
de sala. Sus filtros de candidato no son aún exactos y los peces no llegan al
paquete del runtime, por lo que esto no completa los spawns ni el piso. Se
analizó Decompedia y se confirmó que sus recursos de matching sirven como
referencia metodológica, no como herramientas directas de port ARM. Suite
completa: 187 pruebas; bundle privado VWI2 conserva su hash. Véase
[ITERATION118.md](ITERATION118.md).

La iteración 117 reconstruye la rama `std::_Sort_unchecked` que usa insertion
sort para listas menores de 32 candidatos. En el barrido de 60 layouts, metas
alcanzan como máximo 27 candidatos y fallbacks 12; por ello el reporte marca
exacto el orden de empates para los layouts observados. La composición de las
listas y el estado RNG siguen provisionales. Suite enfocada: 21/21 + 3/3; el
bundle privado 12345/piso 1 conserva hashes y sigue incompleto. Véase
[ITERATION117.md](ITERATION117.md).

La iteración 116 verifica el comparador estricto de coordenadas objetivo
`addStartAndGoal` (`0x00689c60`) contra sus ramas Ghidra con una prueba de todas
las parejas de candidatos. Las 182 pruebas pasan. El orden de MSVC para empates,
el conjunto exacto de candidatos y el estado RNG al entrar en enemigos/jefe
siguen sin validar; seed 12345/piso 1 conserva hashes VWI2/VWE2/VWE3 y el piso
sigue incompleto. No hubo cambio del runtime C ni del VPK. Véase
[ITERATION116.md](ITERATION116.md).

La iteración 115 cierra el ledger RNG de entidades de sala para el catálogo
ACT_ONE: `Boulder` estático (BOULDER_SQUARE), `BoulderBlocker` y waterfall
doodad 20 se clasifican correctamente como RNG-free. Una auditoría de las 287
habitaciones confirma que todos los tipos/subtipos tienen ruta clasificada;
60 layouts también registran `full_room_entity_rng_reconstructed=true`. Esto
no certifica el estado global de `addBoss`: extremos y filtros de enemigos
siguen incompletos. Pruebas enfocadas: 19/19 del layout y 3/3 del barrido.
Véase [ITERATION115.md](ITERATION115.md).

La iteración 114 añade `provisional_enemy_rng_trace`: registra el orden de
salas del modelo de enemigos, sus draws por shuffle/roll y el índice del
recorrido en que Ghidra sitúa la llamada a `addBoss`. Esta auditoría no ejecuta
el jefe y no afirma paridad RNG; el modelo de enemigos aún no ejecuta esa
llamada. También se recuperó el filtro ACT_ONE `y != 0` que excluye la fila
superior del shuffle, y la única llamada conocida a
`addBoss` queda fuera si la sala seleccionada está en esa fila. Las pruebas
enfocadas y el barrido de 60 layouts pasan, y la suite completa queda en
180/180. Seed 12345/piso 1 se regeneró y exportó como VWI2/VWE2/VWE3 privado;
el reporte sigue marcando el piso incompleto y la generación no exacta. WSL
responde `E_ACCESSDENIED`; no hubo build ni ejecución Vita3K porque el runtime
C no cambió.
Véase [ITERATION114.md](ITERATION114.md).

La iteración 113 modela candidatos de spawn con geometría y filtro de entidades
estáticas derivado de `EntityType`, y reconstruye el fallback de cuatro tiles
abiertos más el shuffle en un helper parametrizado por MT19937. Para seed 12345
da 4/3/11 anclas geométricas y 4/0/11 tras entidades estáticas en pisos 0/1/2.
Véase [ITERATION113.md](ITERATION113.md).

La lectura enfocada de `addBoss` posterior a la iteración 112 estableció que
ACT_ONE contempla Goblin King, Dragon y Worm en sus tres slots y que
`spawnedBosses` persiste en el `LevelGenerator` reutilizado entre pisos. La
integración pendiente de jefes debe ocurrir dentro de los pisos y respetar esa
selección/ciclo; el marcador antiguo `boss_transition_pending` describe una
implementación de runtime sin jefes, no evidencia de que el juego original
coloque una arena única al final del tercer piso. Aún falta spawn real,
combate/derrota y validación Vita3K. Véase [ITERATION112.md](ITERATION112.md).

La iteración 112 recupera y registra en cada layout el candidato de sala que
`generateLevel` pasa a `addBoss`: exclusiones, distancia, conteo de tiles
abiertos y desempate x-major. No agrega el jefe al runtime y mantiene
`campaign_complete=false`. Python pasa 173/173; el nuevo bundle local 112
valida tres pisos/66 imágenes y prepara replay de 545 segmentos. El VPK no
cambió (sigue 01.11) y no hay sesión nativa Vita3K validada. Véase
[ITERATION112.md](ITERATION112.md).

La iteración 111 corrige el criterio de cierre: después de dos transiciones,
el runtime informa que completó los tres paquetes de piso, pero mantiene
`campaign_complete=0` porque todavía no integra los jefes nativos que aparecen
dentro de ACT_ONE. La prueba automatizada requiere el nuevo marcador y no valida
ningún jefe. Python pasa 171/171 y VitaSDK construye el VPK 01.11 (835,757 bytes,
SHA-256 `9b1d1bd6be0e7091a9d613791c43c1f37a31fdadce3bbdd8a7ec8d9433991c79`).
Vita3K sigue sin crear una sesión nativa. Véase [ITERATION111.md](ITERATION111.md).

La iteración 105 conserva la embestida normal del Slime de la 104 e incorpora
su salto periódico, gravedad y rebotes aproximados a partir de `Slime::step`
en Ghidra. Prueba rangos del impulso y aterrizaje; las variantes siguen fuera
de alcance porque VWE3 no transporta `SlimeType`. Se añadió una revisión de
decomp.wiki a las [referencias](REFERENCE_REPOSITORIES.md): aporta contexto y
taxonomía, no código de Vagante ni una herramienta para integrar. Validación
y artefactos están en [ITERATION105.md](ITERATION105.md). CTest pasa 12/12,
el VPK 01.05 compila con VitaSDK y la campaña privada valida 113 archivos;
las rutas host de los tres pisos alcanzan salida en 1,741/736/375 ticks. No se
completó Vita3K: el proceso de una corrida visible quedó activo sin ventana ni
log de sesión y se cerró, quedando el resultado `before_native_session`.

La iteración 104 implementa el ciclo de embestida normal del Slime recuperado
con Ghidra: probabilidad 1/20 bajo condiciones de proximidad, aviso de 60
ticks, impulso horizontal 7 por 25 ticks y 150 de enfriamiento. El paquete no
identifica `SlimeType`, por lo que las variantes y el salto periódico aún no
son paridad completa. CTest pasa 12/12 y el VPK 01.04 compila con VitaSDK. Se
regeneraron las rutas v3 de los tres pisos y la campaña privada valida 113
archivos con replay de 545 segmentos. Vita3K cae con `0xC0000005` antes de
crear sesión, incluso usando rutas Windows sin espacios. Véase
[ITERATION104.md](ITERATION104.md).

La iteración 103 añade al Bat su ciclo de carga recuperado con Ghidra:
persecución aérea con aceleración 0.05, aviso de 30 ticks, embestida de 20
ticks a rapidez 4 y enfriamiento de 60 ticks. La persecución directa y el
límite de rapidez son aproximaciones porque el juego delega el movimiento al
PathManager. CTest pasa 12/12. El VPK 01.03 compila con VitaSDK y las rutas de
los tres pisos se regeneraron con el fingerprint actual. Vita3K 0.2.1 termina
con `0xC0000005` antes de crear una sesión, tanto en OpenGL como en Vulkan; la
build histórica 4170 también falla en el mismo punto. El VPK de control 00.25
falla al iniciar en el mismo Vita3K actual. No hay validación nativa de
gameplay. Véase [ITERATION103.md](ITERATION103.md).

La iteración 100 conserva el contador `items_collected` al pasar de piso y
ajusta el diagnóstico para separar daño de formas de arma ya aplicado de
escalado estadístico pendiente. Python pasa 170/170, CTest 12/12 y el VPK
00.98 compila con VitaSDK en WSL; CMake ahora empaqueta correctamente con rutas
de proyecto que contienen espacios (831,570 bytes, hash
`A59A03F04A8C38368CC8A23440CAAE4E6646EB84370A2E343BE0E14924617FF3`). Los
proofs físicos se regeneraron con el fingerprint actual y el replay de tres
pisos se preparó; Vita3K volvió a caer con `0xC0000005` antes de la sesión
nativa. `campaign_complete=false`. Véase [iteración 100](ITERATION100.md).

La auditoría de repositorios públicos no encontró un motor Vagante reutilizable.
`mtolly/vagante-extract` documenta el contenedor VRA, que este proyecto ya
analiza con su parser propio; no se copió su código. `repentogxm` y `D2Vita`
sirven como referencias de recompilación, diagnóstico y pruebas, pero sus
runtimes y licencias no justifican importar módulos al port actual. La build
Vita3K antigua que había pasado el smoke 025 se probó con el VPK 00.97 y también
terminó con `0xC0000005` antes de crear sesión nativa. Véanse
[referencias externas](REFERENCE_REPOSITORIES.md) y
[iteración 099](ITERATION99.md).

La iteración 098 añade al runtime el aviso, impulso, colisión con tiles y
aterrizaje del salto de Spider. Ghidra aporta el ángulo `1.256637 + U·0.1`,
rapidez `3 + 2·U`, gravedad 0.16 y el aviso cercano de 20 ticks. CTest pasa
12/12, incluido un ciclo de salto, daño por contacto durante el aire y
aterrizaje. Estos cambios ya están empaquetados en el VPK 00.97. Las rutas
físicas de los tres pisos alcanzan sus salidas y la entrega privada 097
valida, pero el replay de campaña en Vita3K termina con `0xC0000005` antes de
crear sesión o log nativo. La selección completa de estados de IA, telaraña,
paridad de generación, transiciones jugables y validación en Vita3K/hardware
siguen pendientes; `campaign_complete=false`. Véase
[ITERATION98.md](ITERATION98.md).

La validación 097 regeneró con el simulador actual los proofs físicos v3 de
los pisos 0–2; las rutas llegan a sus salidas en 1,741, 736 y 375 ticks. La
entrega privada 096 valida 113 archivos y genera un replay de campaña de 545
segmentos/3,184 ticks con `player_step` en los tres pisos. CMake/CTest en Ubuntu
pasa 12/12 tests. El smoke automatizado de campaña con VPK 00.96 vuelve a
terminar con `0xC0000005` antes de crear sesión o log nativo en Vita3K. La
campaña sigue sin validación dentro del emulador y `campaign_complete=false`.
Véase [ITERATION97.md](ITERATION97.md).

La iteración 096 compila una regla de contacto de Spider recuperada con
Ghidra: solo durante el estado de salto y con rapidez mínima 0.2, causa 5–10
de daño y evita repetir el golpe al mismo objetivo durante 30 ticks. El estado
de salto aún no lo activa la IA del runtime, por lo que esta regla no equivale
a Spider jugable. El VPK 00.96 compila y el test C compila con advertencias
estrictas para ARM; las 170 pruebas Python pasan. No se pudo ejecutar el test C
host porque no hay compilador de escritorio ni CMake disponible, y Vita3K sigue
fallando antes de crear una sesión. Véase [ITERATION96.md](ITERATION96.md).

La recuperación 094 regenera con WSL los proofs físicos de los tres pisos
usando el fingerprint actual del simulador: 1,741, 736 y 375 ticks; las tres
rutas alcanzan su salida. El paquete privado `_local/delivery-act-one-campaign094`
valida 113 archivos, 3 pisos y 66 imágenes, y produce el replay de campaña de
545 segmentos (3,184 ticks). Vita3K 0.2.1.4183 agotó 90 segundos antes del log
nativo; los logs del emulador y del juego quedaron vacíos, así que no se
acreditan transiciones dentro del emulador. Se usó el VPK 00.93 sin cambios
(SHA-256 `8BF2E7A27E3DCBD940E67846A175B7D33C3267758846955C670E6988B256C4FA`).
Python 3.12 ya estaba instalado en Windows bajo `AppData/Local/Programs`,
aunque el alias `python.exe` de Microsoft Store impedía invocarlo por nombre.
Véase [ITERATION94.md](ITERATION94.md).

La revalidación 095 conserva el mismo VPK 00.93. Vita3K 0.2.1.4170, con Qt
visible y SDL visible, termina con `0xC0000005` antes de crear una sesión; en
0.2.1.4183, Qt visible agota 180 s y el modo consola termina con el mismo
access violation. En todos esos intentos el log nativo permanece vacío, así
que aún no hay evidencia de fallo dentro de Vagante. Ghidra se ejecutó en un
proyecto aislado bajo `_local/ghidra-spider` y produjo un decompilado enfocado
de `Spider::runAI`; confirma los estados idle/walk/jump/web y la lógica de
retarget, pero el pseudocódigo pierde tipos de varios campos y no basta para
recrear todavía el combate con fidelidad. Véase [ITERATION95.md](ITERATION95.md).

La iteración 085 hace determinista la reproducción de entradas en Vita3K al
limitar el catch-up a un tick por cuadro durante pruebas, y permite consumir
correctamente varios segmentos del replay. Pasan 159 pruebas Python y 12 tests
C en WSL. El VPK 00.85 se instaló en la instancia aislada con la replay de tres
pisos preparada. Falta validar las transiciones y el cierre en una nueva sesión
nativa. Véase [ITERATION85.md](ITERATION85.md).

La iteración 084 corrige el panel de inventario para mostrar el rol MISC de
objetos recogidos sin indexar fuera del arreglo de etiquetas; también limita
las filas y avisa cuántos objetos adicionales no caben en el panel. El VPK
00.84 compila con VitaSDK y el archivo fuente modificado pasa compilación ARM
estricta; 159 pruebas Python pasan. El perfil aislado de Vita3K arrancó el
título y cargó el primer piso, pero la ejecución terminó en pantalla de mundo
en el tick 3012 sin transición al segundo piso ni consumir el replay. El replay
se dejó listo para repetir; la campaña completa sigue sin validar. Véase
[ITERATION84.md](ITERATION84.md).

La iteración 083 compila el código actual como VPK 00.83 (830,165 bytes,
SHA-256 `D9DAFF77124974903BF1CC3DDEFDC6086FE241FFEBE3D94B8B231001955A2486`).
La corrección restablece el indicador de salida al cambiar de piso, de modo
que el replay no pueda terminar antes de alcanzar la salida del tercer piso.
Vita3K tampoco creó ventana, log del emulador ni sesión nativa para esta versión.
Véase [ITERATION83.md](ITERATION83.md).

La iteración 082 compila el código actual como VPK 00.82 (830,147 bytes,
SHA-256 `76fb58b0440b029ecb6ba84a9ceea9417f47b608b0f6fad69398b9690bf22bef`).
El bundle privado de campaña valida 100 archivos; una auditoría adicional
confirma coincidencia de tamaño y SHA-256 para los 74 recursos originales
incluidos. La replay tiene 545 segmentos, 3,184 ticks y trazas `player_step`
en los tres pisos. La biblioteca de la instancia aislada de Vita3K muestra el
título instalado 00.82, pero no se pudo seleccionar para abrir una sesión
nativa; la transición de campaña sigue sin verificarse en el emulador. La
última evidencia funcional sigue siendo la iteración 081: el probe nativo
llega al marcador de salida en cada piso, pero no prueba las transiciones
dentro del emulador. Generación exacta por semilla y enemigos de los pisos 1–2
siguen provisionales. Véase [ITERATION82.md](ITERATION82.md),
[ITERATION81.md](ITERATION81.md) y [revisión de las referencias de port](PORTING_REFERENCE_REVIEW.md).

La iteración 080 permite exportar trazas VTR1 desde el probe C de `player_step`, sellarlas en proofs v3 y consumirlas desde la replay de campaña si coinciden los hashes del piso y del simulador. El VPK 00.80 compila y pasa `check_vpk.py`; 159 pruebas Python pasan. El smoke aislado de 00.80 agotó 45 s antes de iniciar sesión nativa y confirmó que la campaña disponible solo produce rutas estimadas. WSL y la falta de compilador host impiden generar los proofs nuevos. Véase [ITERATION80.md](ITERATION80.md).

La iteración 078 añade una replay automatizada que intenta recorrer las tres salidas y exige ambos cambios de piso y el cierre de ACT_ONE en los logs. El VPK 00.78 compila, pero el clon de Vita3K agotó 45 s o terminó con `0xC0000005` antes de crear una sesión nativa. Esta infraestructura aún no acredita la física ni completa el mundo. Véase [ITERATION78.md](ITERATION78.md).

La iteración 076 conserva el ataque de Goblin reconstruido desde `goblin-stab`
y añade la primera ruta física de ACT_ONE probada con el `player_step` real del
runtime. GCC en WSL simuló la semilla 12345 durante 1,741 ticks y 14 saltos;
el jugador llegó vivo y apoyado al marcador de salida tras 56 tiles. El proof
se liga por SHA-256 al VWI2 y al VWE2. El bundle privado valida 100 archivos y
marca `native_player_route_verified=true` para el piso 0; pisos 1–2 y las
transiciones siguen sin validar físicamente.

El VPK 00.76 se compiló con VitaSDK para Windows (830,127 bytes, SHA-256
`d2a4dbf27f9d6737f5482e04c4815c6748f58edfd08825f3a29b4956c970aff7`). El probe
físico se compiló y ejecutó con GCC de WSL; VitaSDK Linux no está instalado en
esa distro. Vita3K agotó 45 s antes de crear sesión o logs, así que el gameplay
aún no se valida en emulador. La generación ACT_ONE tampoco tiene paridad
exacta de seed y el primer mundo sigue incompleto. Véase
[ITERATION76.md](ITERATION76.md) para alcance y evidencia.

El probe físico ahora compila con GCC estricto en WSL y completó el piso 0. El smoke aislado de Vita3K 073 agotó 45 s antes de generar
logs de la app o del emulador; no valida la carga ni el gameplay.
La revisión de pools confirma categorías, subfamilias y cinco registros de
espada; la selección de loot y su integración con cofres ya usa el paquete VLT1
externo. La inspección de plantillas no encontró objetos colocados en ACT_ONE
que puedan reutilizarse: los objetos fijos detectados pertenecen a una sala
especial de otro acto. El inventario nativo usa 35 ranuras y reglas de stack y
tipos únicos; esas reglas y la paridad completa de objetos/equipos siguen
pendientes. La replay Vita3K 00.53 volvió a agotarse a 90 s en
SDL/Vulkan y Qt/OpenGL, antes de crear log nativo. La compilación ARM estricta
y las 119 pruebas Python sí pasan. Detalles y evidencia en
[GENERATION_RECOVERY.md](GENERATION_RECOVERY.md) y
[ITERATION53.md](ITERATION53.md).

Actualización Iteración 053: las flechas ya consultan las mismas cajas de
Goblin, Bat, Lurker y Slime recuperadas de sus constructores que usan melee,
contacto y movimiento. La generación sigue provisional y la replay de gameplay
en Vita3K continúa pendiente. VPK `00.53` compilado. Véase
[ITERATION53.md](ITERATION53.md).

Actualización Iteración 051: los builds VitaSDK y CMake desactivan FMA en el
runtime de movimiento para mejorar la consistencia de física y replay entre
compilaciones. Véase [ITERATION51.md](ITERATION51.md).

Actualización Iteración 050: la Boulder cuadrada ahora daña al jugador una vez
por impacto al superar el umbral de velocidad observado en Ghidra; un impacto
letal queda registrado como causa Boulder. El empuje, filtros completos de
inmunidad y daños a enemigos siguen pendientes. Véase [ITERATION50.md](ITERATION50.md).

Actualización Iteración 049: el Boulder cuadrado ya cae tras el retardo de seis
frames con gravedad y velocidad inicial observadas en Ghidra, y se detiene
sobre el primer tile sólido. El resolvedor de tiles es aproximado; daño a
unidades y ciclo de sacudida/reaparición siguen pendientes. El VPK `00.49`
compila, pero el smoke aislado de Vita3K falla antes de iniciar el juego por
una excepción interna de lectura nula del emulador. Véase
[ITERATION49.md](ITERATION49.md).

Actualización Iteración 048: ACT_ONE instancia y dibuja el Boulder cuadrado
asociado a cada BoulderTrap cuando encuentra un tile sólido ancla por encima,
como hace `BoulderTrap::setLevel`. El paquete privado 050 incluye el sprite
original. Véase [ITERATION48.md](ITERATION48.md).

Actualización Iteración 047: se conserva el armado inicial de 60 frames de
BoulderTrap y se modela la orden de liberar la roca en el sexto frame tras la
activación, según Ghidra. La simulación del actor Boulder, su movimiento, daño y
validación en Vita3K siguen pendientes. VPK `00.47` compilado; véase
[ITERATION47.md](ITERATION47.md).

Actualización Iteración 046: se añadió la secuencia de reentrada a la prueba
de BoulderTrap, incluyendo el caso de permanecer sobre la placa durante los
60 ticks de armado y activar solo tras salir y volver a pisar. VPK `00.46`
compilado. Véase [ITERATION46.md](ITERATION46.md).

Actualización Iteración 045: el estado de BoulderTrap ya no reutiliza campos
del proyectil de dardos. Se validó el retardo de inicialización de 60 ticks y
la activación única con 119 pruebas Python y compilación C estricta Vita; se
generó VPK `00.45`. La placa todavía no lanza una roca ni aplica daño y Vita3K
continúa pendiente. Véase [ITERATION45.md](ITERATION45.md).

Actualización Iteración 044: ACT_ONE ahora dibuja la BoulderTrap con sus dos
estados y ejecuta la retracción inicial de 60 ticks y activación única de su
placa, a partir del pseudocódigo Ghidra. VPK 00.44 compilado; 119 pruebas
Python pasan y el manifiesto de campaña 049 valida 68 archivos. El lanzamiento
automático en Vita3K 0.2.1.4183 aún no produce sesión nativa; el daño/liberación
de Boulder sigue pendiente y no se reclama como trampa completa. Véase
[ITERATION44.md](ITERATION44.md).
Actualización Iteración 042: el VPK 00.42 se compiló con el VitaSDK y conserva
el título `VAGV00006`. La entrega privada 048 incluye pruebas `player_step`
ligadas por hash a cada uno de los tres pisos; las rutas terminan en 664, 809 y
401 ticks. El cargador de campaña implementa la transición de salida al
siguiente piso; el smoke estaba configurado para verificarla, pero Vita3K
terminó con `-1` tras 60 segundos, antes de crear una sesión nativa. La
transición sigue sin validar en el emulador. Las 119 pruebas Python
pasan y el manifiesto valida 68 archivos. Paridad exacta de generación, loot,
comportamiento completo de entidades, transiciones verificadas en Vita3K y el
jefe siguen pendientes. Véanse [ITERATION42.md](ITERATION42.md),
[ITERATION40.md](ITERATION40.md), [ITERATION39.md](ITERATION39.md),
[ITERATION38.md](ITERATION38.md),
[ITERATION37.md](ITERATION37.md) y
[GENERATION_RECOVERY.md](GENERATION_RECOVERY.md).

Actualización Iteración 035: las flechas ya pueden dañar y eliminar enemigos;
las transiciones conservan salud, atributos, equipo y munición. CTest host 11/11,
Python 117/117 y VPK `00.35` compilado con VitaSDK en WSL. El smoke de Vita3K
terminó con `0xC0000005` antes de crear sesión nativa, así que el arranque y las
transiciones siguen sin validar en el emulador. El primer mundo aún carece de
botín real, trampas activas, jefe y paridad exacta de generación. Véase
[ITERATION35.md](ITERATION35.md).

Los tres pisos de seed 12345 completan rutas físicas con `player_step` (664,
809 y 401 ticks). Los pisos 1–2 usan apoyos y escaleras provisionales, no
generación con paridad. La entrega privada 047 valida 68 archivos y conserva las
rutas ligadas a hashes. Detalles de esa prueba en [ITERATION34.md](ITERATION34.md).

La Iteración 033 consigue recorrer el piso base con `player_step` usando un
controlador predictivo: seed 12345, salida en 656 ticks y 10 saltos. El piso 1
no completa; sus caminos tile-only incluyen ascensos que necesitan modelarse
como saltos, y una ruta solo por escaleras no existe. Piso 2, transiciones y
Vita3K con paquete actualizado siguen pendientes. Ver [ITERATION33.md](ITERATION33.md).

La fuente contiene el ensamblador privado ACT_ONE para tres pisos de 80×40,
con terreno, agua, entidades, enemigos provisionales y salida. La entrega de
datos 045 **no es un VPK 045 ni un mundo completo**: sus informes mantienen
`floor_complete=false`, `exact_generation_parity=false`,
`entities_instantiated=false`, `native_player_route_verified=false` y
`native_exit_transition_verified=false`. Sus rutas conectan tiles; no modelan
la física real del jugador. Los datos comerciales permanecen en `_local` y no
se incluyen en el VPK.

Estado de compilación: existe el VPK 00.32 en
`_local/build-act-one-032/Vagante2Vita-iteration032.vpk`. No representa un
VPK 045: 045 identifica los datos/layouts generados. La transición entre los
tres paquetes está escrita en el runtime, pero no se ha comprobado ejecutando
el juego.

Verificación repetida en esta máquina: la suite Python con `unittest` pasa
114/114; el barrido del generador valida 60 combinaciones (semillas 0–19,
pisos 0–2) para dimensiones, geometría de aparición, capacidad y ruta de tiles.
Estas pruebas no verifican movimiento `player_step`, IA/combate completo ni
transiciones nativas. No hay compilador C host disponible y WSL responde
`E_ACCESSDENIED`, así que el probe físico no puede ejecutarse aquí. El smoke
Vita3K tampoco está validado: la copia privada no abre ventana ni genera log;
el log de `G:\Vita3k` corresponde a una versión anterior. El harness exige
coincidencia de la iteración en el juego y en Vita3K para rechazar logs viejos.

El alcance pendiente para declarar ACT_ONE completo conserva los tres pisos y
el jefe: física y ruta automatizadas; IA, ataques, daño y muerte originales;
cofres con loot e inventario; trampas, rocas, tiendas y objetos activos;
generación reproducible; transiciones y desenlace del jefe; y una ejecución
válida en Vita3K (más validación final en Vita física). El trabajo de
reconstrucción y sus fuentes están en [GENERATION_RECOVERY.md](GENERATION_RECOVERY.md);
la última compilación descrita en detalle está en [ITERATION32.md](ITERATION32.md).


# Iteración 028: control de cofre ACT_ONE corregido

Ghidra confirma que `Player::moveUp` abre el cofre tocado y que el constructor
usa una caja de 12×9 px. El runtime deja de abrir cofres con melee: una
pulsación nueva de ↑ al tocarlo cambia su estado visual una sola vez. Se
compiló el VPK 00.28 y las fuentes runtime pasan compilación ARM estricta; la
suite Python pasa 103/103. El probe C de recorrido físico está añadido al build host y compila para ARM; falta ejecutarlo. Vita3K y hardware siguen sin validar el cambio.

El primer mundo jugable sigue incompleto: faltan generación/ensamblado final,
loot real, trampas y daño, enemigos, salida/transición entre pisos y una
validación automatizada completa. El VPK no trae recursos comerciales. Ver
[ITERATION28.md](ITERATION28.md).


# Avance de reconstrucción 029 (VPK vigente 025)

Sorteos float32 y barajado de la compilación Windows recuperados. Salas secretas
y pedestales integrados después del tesoro, antes de seleccionar plantillas.
42 pruebas Python pasan; 4000 resultados decimales coinciden bit a bit con
referencia C++ de redondeo. 300 grafos con recursos privados: 12000 selecciones
sin fallos de unión local. [Evidencia y límites](GENERATION_RECOVERY.md).
Pendientes: inicializar tiles y entidades, bordes, sellado de secretos,
entrada/salida, recorribilidad global, transición y juego completo.
El VPK sigue siendo 025; esta recuperación no se presenta como piso jugable.

# Avance de reconstrucción 028 (VPK vigente 025)

Etapa de tesoro cerca de salida recuperada. Selector geométrico por columnas,
frecuencias y reintentos implementado para salas simples. 300 análisis con
plantillas originales sin fallos de unión local; 29 pruebas pasan.
[Evidencia y límites](GENERATION_RECOVERY.md). Secretos y pedestales todavía
alterarán la selección definitiva; primer piso jugable aún no generado.

# Avance de reconstrucción 027 (VPK vigente 025)

Ramas con ciclos y etapa de tienda ACT_ONE recuperadas, preservando sorteos,
límites y tipos originales. 300 grafos conectados y tipos disponibles en la
base del usuario; 22 pruebas pasan. [Evidencia y comandos](GENERATION_RECOVERY.md).
Faltan tesoros, salas especiales restantes, selección geométrica y montaje;
todavía no se ha generado el primer piso jugable ni comprobado paridad PC.

# Avance de reconstrucción 026 (VPK vigente 025)

Motor MT19937 y distribución original contrastados con el PE; camino principal
ACT_ONE reconstruido para floorIndex 0–2, preservando consumos aleatorios.
Selector por menor uso recuperado, incluidos intentos geométricos rechazados.
[Comandos y evidencia](GENERATION_RECOVERY.md). Aún faltan ramas, salas especiales,
ensamblado y transición; no hay primer piso nuevo jugable ni equivalencia PC probada.

# Estado vigente: iteración 025

VPK ejecutado y validado automáticamente en Vita3K con secuencia externa,
instancia/datos separados y logs por ejecución. Perfil genérico de teclado
aplicado al emulador principal. [Uso, resultados y alcance](ITERATION25.md).
La intro original se conserva; generación y juego completo continúan pendientes.

# Estado vigente: iteración 024

VWI2 admite dimensiones/entrada/salida variables; cámara horizontal y vertical,
terreno con caché sin segunda copia completa y comprobación de tamaño por mapa.
Intro original conservada. ACT_ONE 80×40 probado con fixture local de geometría,
no ensamblado todavía con habitaciones originales. [Instalación y cobertura](ITERATION24.md).
Generación, transición y juego completo continúan pendientes.

# Avance de reconstrucción 024 (VPK vigente 023)

Validadas 780 habitaciones originales y reconstruida la comprobación vertical
de enlaces simples. Primer piso ACT_ONE: cuadrícula 8×5 de habitaciones 10×8,
80×40 tiles. Cargador/cámara, ensamblado, RNG e interacciones siguen pendientes.
[Datos recuperados y comandos](GENERATION_RECOVERY.md). No hay nuevo VPK aún.

# Estado vigente: iteración 023

Arco de Rogue con carga, apuntado discreto, disparo, munición y colisión de
flechas con terreno. Datos y sprites originales permanecen externos al VPK.
Recuperación de flechas, daño a entidades, magia y generación de pisos pendientes.
[Instalación y validación](ITERATION23.md). Objetivo completo aún no alcanzado.

# Estado vigente: iteración 022

Ataque base player-stab ejecutable con Cuadrado, datos externos originales,
animación de personaje/arma y polígonos activos. Rod incluye retención de carga;
aplicación de daño, efecto mágico y combate con entidades todavía pendientes.
[Instalación y cobertura](ITERATION22.md). Objetivo completo aún no alcanzado.

# Estado vigente: iteración 021

Estadísticas derivadas intrínsecas y equipo/skills iniciales recuperados. SELECT
abre ficha consultable de tres páginas con física detenida. No ejecuta combate
ni reemplaza inventario/menús originales completos. Recursos externos iguales a 020.
[Instalación y validación](ITERATION21.md). [Auditoría: objetivo aún incompleto](GOAL_AUDIT21.md).

# Estado vigente: iteración 020

Integradas las 22 entidades de INTRO mediante archivo privado externo. Se
dibujan antorchas, flores y luciérnagas; marcadores sin Sprite y trofeo sujeto
 a desbloqueo no se dibujan. Trayectorias, luces e interacciones aún pendientes.
[Instalación y pruebas](ITERATION20.md). Objetivo completo activo, no alcanzado.

# Estado vigente: iteración 019

Comparados tres perfiles de menú en ambas resoluciones. Perfil preparado nuevo
con capas por filas y colores precalculados; predeterminado ligero para priorizar
el recorrido jugable. Sin métricas internas ni recursos comerciales en VPK.
[Pruebas y límites](ITERATION19.md). El juego completo sigue pendiente.

# Estado vigente: iteración 018

Menú con fondo animado por defecto; R permite comparación externa con perfil
ligero. Sin métricas internas ni recursos del juego dentro del VPK.
[Instalación](ITERATION18.md). [Dependencias de generación recuperadas](GENERATION_RECOVERY.md).

# Estado vigente: iteración 017 y alcance ampliado

Primera adaptación de agarre/subida/suelta/salto de bordes, aún sin paridad PC
completa. Recursos externos; VPK limpio y sin métricas internas.
[Controles y validación](ITERATION17.md).

## Objetivo del mundo completo

1. Recuperar generación por actos y semilla desde generateLevel y sus llamadas.
2. Cargador de habitaciones de tamaños distintos y enlaces entre habitaciones.
3. Entidades/interacciones y transición INTRO al primer piso.
4. Combate, objetos, equipo, atributos derivados y progreso.
5. Ampliar actos, variantes, enemigos/jefes y guardado con validación física.

No se considera completo por mostrar más mapas: cada transición, movimiento
y sistema debe ejecutarse y verificarse. El estado actual es una intro parcial.

# Estado vigente: iteración 016

015 confirma recorrido físico del mundo con Knight/Rogue y ~58–59 FPS.
016 corrige aterrizajes, añade pausa START y retira métricas internas.
VPK sin recursos del juego; world y caché se entregan aparte.
[Instalación y pendientes](ITERATION16.md).

# Estado vigente: iteración 015

La prueba física 014 no entró al mundo: faltaba intro-world.vwi. La entrega local
015 incluye el mundo original en su VPK y una alternativa de carga app0:world.
Permite probar el recorrido implementado (movimiento, colisiones y atributos).
Falta validar entrada y rendimiento físico; combate/interacciones/primer piso
procedural siguen pendientes. [Instalación y alcance](ITERATION15.md).

Los apartados siguientes conservan el historial de decisiones y pendientes.

# Roadmap hacia Vagante en Vita

## Estado actual: iteración 093

El runtime y el exportador ahora aceptan Spider (ID 14), que la generación
ACT_ONE produce en pisos 1 y 2; la seed 1 del piso 1 ejercita la ruta. El
delivery incluye idle, walk, jump, hurt y webshot y el preflight los comprueba
siempre. La IA, el salto, la telaraña y el daño siguen usando un modelo
provisional. Pasan 170 pruebas Python y cuatro archivos C compilan con VitaSDK
ARM estricto.

El paquete privado 093 valida 113 archivos, tres pisos y 66 imágenes. El VPK
00.93 mide 830,445 bytes (SHA-256
`8BF2E7A27E3DCBD940E67846A175B7D33C3267758846955C670E6988B256C4FA`). El
smoke estándar de Vita3K agotó 90 segundos antes de crear sesión nativa o logs.
El replay de campaña se detuvo en la preparación porque los proofs conservan
un fingerprint previo; WSL no permitió enumerar sus distribuciones (`E_ACCESSDENIED`)
para regenerarlos. No se valida gameplay. Siguen pendientes el combate nativo
de Spider, paridad de generación, jefe, proofs actuales y transiciones
observadas. Véase [ITERATION93.md](ITERATION93.md),
[ITERATION92.md](ITERATION92.md) y
[GENERATION_RECOVERY.md](GENERATION_RECOVERY.md).

## Estado después de la iteración 3

El usuario comprobó el título 00.02 en Vita: carga correcta, pantalla estática. 00.03 está compilada y comprobada en escritorio; requiere prueba física. Carga 22 PNG desde data.vra, verifica los 2.705 registros, anima fondo/caravana/luciérnagas y alterna dos framebuffers. New Game abre una selección parcial de las tres clases base tras 90 pasos. Confirmar una clase registra el bloqueo: falta reconstruir el arranque del mundo.

[Prueba y diagnóstico 00.03](ITERATION3.md). El log registra recursos, estados, rendimiento CPU y tareas pendientes. Los datos de render no representan rendimiento de gameplay.

## Hitos pendientes

1. **Validar título en Vita:** comprobar carga, colores, selección y salida; recuperar runtime.log. Medir memoria máxima y separar CPU de espera de vblank. En 00.03 los tiempos CPU excluyen espera/presentación; medir draw_fps en el título y vista de clases.
2. **Completar presentación:** recuperar Sprite/Animation, atlas, tintes día/noche, intro y temporizadores; comparar fotogramas con PC. Signpost ya se carga; recuperar su posición exacta, fuente y audio originales. Evaluar GPU con mediciones.
3. **Selección de personaje:** la vista parcial ya cambia entre tres clases; reconstruir el flujo Client/Server, desbloqueos, fondos, variantes y guardados. Criterio: elegir personaje y obtener semilla reproducible.
4. **Una habitación funcional:** recuperar RNG, generación mínima, tiles, Player/Entity, movimiento, colisión y cámara. Comparar estados con semilla y entradas fijas en PC y luego Vita.
5. **Combate y progreso mínimo:** enemigo, daño, objetos, transición y persistencia. Criterio: recorrido mínimo sin corrupción.
6. **Ampliar y optimizar:** generación completa, clases, enemigos, objetos y jefes. Medir CPU/GPU/memoria en consola; cache acotada, atlas y streaming de audio.
7. **Funciones adicionales:** opciones, localización, controles y red; adaptar dependencias PC/Steam.

No hay fecha ni porcentaje fiable. Los 30.568 cuerpos de Ghidra incluyen bibliotecas y no equivalen a funciones reconstruidas.

## Evidencia local

MainMenu: constructor 0x006a2ca0, updateStrings 0x006a3c00, drawBackground 0x006a4ba0, draw 0x006a5b50, up 0x006a6790, down 0x006a67d0, action 0x006a6810. Pseudocódigo exclusivamente en _local/decompiled. La siguiente investigación debe seguir readyToStartGame en Client::step y reconstruir la inicialización del mundo, además de completar PlayerMenu.

## Conservación desde 004

Entregas en carpetas independientes y logs por sesión, sin reemplazar anteriores. Ver [ITERATION4.md](ITERATION4.md). La jugabilidad pendiente sigue igual.

## Optimización 005

Base de render: 320×180 o 640×360 con comparación por L y tiempos separados de composición/escalado. Caché privada de menú de 153 KB evita inflar todo el VRA al arrancar. Ver [ITERATION5.md](ITERATION5.md). Próximo paso: obtener logs físicos de ambos perfiles, cuantificar CPU residual del escalado y decidir si conviene backend GPU o formato de texturas; no extrapolar benchmark de escritorio.

## Presentación 006

Icono, fondo LiveArea, puerta de inicio y splash integrados; VAGV00006 independiente. Ver [ITERATION6.md](ITERATION6.md). El usuario reporta mejora con 005; los logs adjuntos son 003 y falta cuantificar 005/006 con logs numerados. Próximo avance funcional: reconstruir arranque de mundo y completar el flujo de selección, conservando mediciones de composición/escalado.

## Estándar desde 007

Mantener TITLE_ID VAGV00006 y nombre Vagante2Vita; incrementar sólo versión/iteración. Una aplicación actualizada en Vita, entregas y logs separados en PC/consola. Ver [ITERATION7.md](ITERATION7.md). Abrir la carpeta de entrega al finalizar cada iteración.

## Medición y avance 008

Los logs físicos de 006 confirman carga de 0,644 s, dibujo de 9,2–9,8 FPS y composición de 69–72 ms más escalado de 23–25 ms. 008 aplica caché de paisaje, animación de sprites y escalado desde RAM; falta medición física. Ver [ITERATION8.md](ITERATION8.md). La siguiente tarea funcional es recuperar INTERMISSION de 40×24 tiles desde rooms.json, resolver el grupo INTRO y sus seis campos por tile; después, colisión, jugador y cámara. [Criterios y funciones pendientes](FIRST_GAMEPLAY.md). El mundo jugable sigue pendiente.

## Análisis independiente 009

Validadas 15 habitaciones INTERMISSION de 960 tiles y generadas vistas privadas de geometría. Clasificación base de sólidos/plataformas identificada en generateCollisionFlags; faltan carga/transformaciones, enum INTRO, caras de colisión y física. [Resultados](ANALYSIS9.md). La aplicación de prueba sigue en 008.

## Base nativa 010

INTRO=4 confirmado por PDB y selección por acto/tipo. Implementados paquete privado de 960 tiles, campos de capas visuales, flags/caras de colisión y atributos base de tres clases. Pruebas nativas aprobadas. [Detalle](ANALYSIS10.md). Próximo: autotiling/render, entidades y movimiento del jugador sobre la habitación real. Objetivo jugable activo.

## Terreno 011

Autotiling reconstruido, CSV nativo y composición privada de los atlas de la intro generados. Pruebas aprobadas; falta comparación con PC, doodads, iluminación y personaje. [Resultados](ANALYSIS11.md).

## Movimiento 012

Cuerpo nativo y subconjunto de andar/saltar/agacharse/escaleras sobre el mapa original. Atributos y salud base conectados; equivalencia física completa y movimientos adicionales pendientes. [Alcance](ANALYSIS12.md).

## Integración Vita 013

Primera prueba interactiva de la habitación INTRO original con andar, salto, agacharse, plataformas, escaleras y HP/atributos base. Recursos privados de world separados. [Entrega y limitaciones](ITERATION13.md). Faltan rodar, bordes, combate/equipo/habilidades, interacción y estadísticas completas; objetivo automático activo.


## Iteración 014 — optimización de presentación

Salida por filas modificadas, escalado en RAM y terreno precalculado. Objetivo 60 Hz; pendiente medir FPS reales en Vita y picos en cambios de pantalla. Véase ITERATION14.md.
## Iteración 054 — armas VWA

VWA carga 12 definiciones melee recuperadas de `attacks.json` y mapeadas por
Ghidra; 121 pruebas Python y build VitaSDK aprobados. Vita3K no produjo log
nativo antes del timeout. Pendientes: integrar animaciones de maza/hacha,
selección y generación de botín, recogida/equipamiento, y validar la salida de
ACT_ONE dentro del emulador. Véase [ITERATION54.md](ITERATION54.md).

## Iteración 055 — pools de botín

Recuperadas y verificadas las 170 filas directas de `ItemRarity` en 18 pools,
con salida privada desde Ghidra y la copia local del ejecutable. El ID 62 queda
identificado como FLAIL. Suite Python: 123/123; paquete VPK 054 válido. Dos
arranques aislados recientes de Vita3K no produjeron logs de aplicación y no
confirman gameplay. Sigue pendiente integrar la selección, materialización y
recogida del botín en ACT_ONE. Véase [ITERATION55.md](ITERATION55.md).

## Iteración 056 — sprites de objetos

Relacionados los 170 tipos directos de los pools con los atlas de `Item` y sus
índices de sprite; 136 son explícitos en Ghidra y 34 mantienen el default -1.
El extractor comprueba enum y cobertura, sin añadir recursos comerciales al
VPK. Falta conectar las hojas, dibujar objetos, simular su caída y recogerlos.
Véase [ITERATION56.md](ITERATION56.md).

## Iteración 057 — build actual y botín de cofres

La VPK 057 se compila y pasa `check_vpk.py`; Vita3K agota 60 segundos sin
sesión nativa. Ghidra confirma el flujo `POOL_CHEST` → selección por nivel,
rareza y unicidad → creación de Item, pero el runtime aún no materializa ni
recoge botín. Véase [ITERATION57.md](ITERATION57.md).

## Iteración 058 — grafo de botín auditado

El extractor confirma que `POOL_CHEST` incluye armas, armadura, otros objetos
y varitas. Los pesos quedaron pendientes en esa auditoría inicial; véase
[ITERATION58.md](ITERATION58.md).

## Iteración 059 — pesos estáticos de cofres

El listing y el PE recuperan pesos de categoría 30/60/4/6 y los verifican
contra el PDB. Véase [ITERATION59.md](ITERATION59.md).

## Iteración 060 — grafo de pesos anidado

Siete vectores `ItemPoolRarity` están extraídos del constructor y asociados a
sus pools por ID PDB, incluidos los subpesos de armas y armadura. Faltan la
selección de filas por nivel/unicidad, la creación y recogida de objetos, y la
prueba nativa en Vita3K. Véase [ITERATION60.md](ITERATION60.md).

## Iteración 061 — paquete VLT1 de botín

El extractor ya separa las 16 pociones positivas de las 10 negativas; se
empaquetan 19 pools hoja, 170 filas y 32 aristas en `_local/item-loot.vlt`.
La cobertura recursiva del cofre pasa para niveles 0–10 y la suite Python pasa
132 pruebas. El runtime aún no carga ni usa el paquete.
Véase [ITERATION61.md](ITERATION61.md).

## Iteración 062 — selección y recogida de botín

El runtime ACT_ONE carga `VLT1` v2, elige por nivel/peso/únicos, dibuja drops
con los atlas del bundle privado y permite recogerlos al inventario. Build
VitaSDK y auditoría VPK pasan; suite Python: 136/136. No se ejecutaron las
pruebas C porque CMake/CTest no está instalado. Vita3K aislado se agotó a 60 s
sin sesión nativa ni logs tanto con el paquete ACT_ONE como INTRO, así que
sigue sin evidencia de arranque. El piso completo aún requiere paridad del
mapa, trampas/combate y transición probada.
Véase [ITERATION62.md](ITERATION62.md).

## Iteración 063 — loot persistente entre pisos

La carga de candidatos para los pisos 1 y 2 vuelve a cargar VLT1 y asigna el
nivel de loot antes del cambio de mundo; la preparación de campaña conserva el
paquete en el manifiesto. Bundle privado de 3 pisos: 78 archivos validados y
route proofs de los mundos incluidos. Suite Python 136/136, build VitaSDK y
auditor VPK pasan. La campaña y sus mapas siguen provisionales, no exactos. El
smoke visible de Vita3K volvió a agotar 60 s sin sesión ni logs. Véase
[ITERATION63.md](ITERATION63.md).

## Iteración 064 — validación del paquete de loot de campaña

La preparación de ACT_ONE ahora inspecciona el contenido VLT1 v2 completo,
incluidos tamaños, límites del runtime, descriptores y el pool raíz de cofres;
rechaza campañas sin una tabla válida. La prueba de empaquetado verifica la
tabla conservada y el hash en `campaign.json`, además de rechazar una tabla
truncada. El VPK 064 compila con VitaSDK y conserva las siete entradas válidas.
La suite Python completa pasa 137 pruebas y se regeneró un bundle ACT_ONE de
78 archivos. Vita3K expira sin logs tanto por instalación SDL como por carga
directa de consola; la ejecución automatizada sigue sin poder verificarse.
Véase [ITERATION64.md](ITERATION64.md).

## Iteración 065 — entidades pendientes reportadas por tipo

El log VWE2 ahora cuenta como pendientes únicamente BreakableWall y Boulder
independiente; los tres pisos actuales no contienen esos tipos y el smoke exige
`behavior_pending=0`. VPK 065 compilado con VitaSDK; suite Python 137/137.
Véase [ITERATION65.md](ITERATION65.md).

## Iteración 071 — BreakableWall en ACT_ONE

El runtime inicializa `BreakableWall` con 14 HP, acepta golpes de melee y al
romperlo elimina el tile asociado y las paredes conectadas ortogonalmente;
recalcula colisiones y autotiles. El bundle ACT_ONE incluye su sprite de
16×16. La suite Python pasa 140 pruebas y el VPK 071 compila con VitaSDK. El
test C del grupo de paredes quedó añadido, pero no se pudo ejecutar porque no
hay compilador C host. El smoke con el clon aislado de Vita3K agotó 30 segundos
sin sesión nativa y sin logs del emulador. La paridad de daño, el sorteo de
rotación, los efectos y la metadata de tiles indestructibles siguen pendientes.
Véase [ITERATION71.md](ITERATION71.md).

## Iteración 072 — Boulder independiente en ACT_ONE

El Boulder kind 22 ahora permanece en sacudida 120 frames, inicia la caída con
velocidad vertical 0.32, acelera por gravedad y aterriza sobre el terreno; su
impacto puede dañar una vez al jugador. Se incluye el sprite de sacudida en el
paquete externo. El VPK 072 compila y la suite Python pasa 140 pruebas; las
pruebas C compilan con advertencias estrictas para ARM. La corrida aislada de
Vita3K terminó por timeout sin interfaz ni logs, así que la versión sigue sin
validación en emulador. La asignación del subtipo de sacudida es provisional
porque los registros de sala no codifican el subtipo; ángulo, aleatoriedad,
sonido, partículas y reglas de interacción completas siguen pendientes. No se
declara completo el primer piso. Véase [ITERATION72.md](ITERATION72.md).

## Iteración 073 — sprites de enemigos y paquete ACT_ONE

El bundle privado ahora contiene `item-loot.vlt` y los siete atlas exigidos al
abrir ACT_ONE, además de las hojas stand/move/air/hurt de Goblin, Bat, Lurker y
Slime. El renderer dibuja esos sprites animados en lugar de los rectángulos de
color; la IA, ataque y daño de enemigos continúan provisionales. Se detectó que
el bundle manual 072 omitía loot obligatorio, así que 073 se generó con
`prepare_world_bundle.py` y `prepare_act_one_campaign.py`; valida 94 archivos y
preserva pisos 0–2 con generación provisional. El VPK 073 compila, contiene las
siete entradas esperadas y la suite Python pasa 140/140. Los archivos C
modificados compilan con advertencias estrictas para ARM. El smoke de salida
ACT_ONE configuró 34 pasos y 56 tiles de ruta, pero Vita3K agotó 45 s antes de
crear sesión o logs; no demuestra arranque ni transición. No se declara
completo el primer mundo. Véase [ITERATION73.md](ITERATION73.md).

## Iteración 151 — rutas ACT_ONE regeneradas con el simulador actual

El runner tolera el paquete actual del piso 0 sin jefe, resuelve los recursos
compartidos desde la raíz de campaña y firma proofs con las mismas fuentes que
el verificador. Después de derrotar al Goblin King en el piso 2, vuelve a
calcular la ruta hacia la salida desde la posición real del jugador. Los tres
pisos pasan el probe host y generan proofs v3 actuales; el replay de campaña se
prepara con 607 pasos/4,140 ticks y el bundle valida 142 archivos. La suite
Python pasa 204/204 y las pruebas C host 13/13. Vita3K expira a los 120 s antes
de una ventana o sesión nativa incluso con el perfil completo aislado; no se
declara verificada la campaña allí. Generación exacta, paridad de IA/RNG y
transiciones en Vita3K siguen pendientes. Véase
[ITERATION151.md](ITERATION151.md).

## Iteración 085 — replay determinista y campaña ACT_ONE

El replay limita la recuperación a un tick de física por cuadro y consume
segmentos consecutivos correctamente. La suite actual pasa 162 tests Python y
66 subtests; los 12 tests C del runtime pasaron en la ejecución WSL anterior.
El VPK 0.85 se recompiló con VitaSDK Windows al fallar WSL con
`E_ACCESSDENIED`; sus siete entradas, incluido `eboot.bin`, coinciden con el
VPK previo. El paquete privado valida 108 archivos y el preflight comprueba
56 PNG, los tres pisos y sus entidades/enemigos. Vita3K Qt, SDL y consola
expiran antes de crear ventana, log o sesión nativa; por ello no se han
verificado el renderer ni las transiciones. ACT_ONE sigue incompleto:
generación exacta, paridad de enemigos e inventario, jefe, transiciones nativas
y pruebas en Vita real siguen pendientes. Véase [ITERATION85.md](ITERATION85.md).

## Iteración 101 — ataque de telaraña del Spider

El Spider selecciona un lanzamiento de telaraña al azar cuando está a menos de
64 px del jugador, usa el proyectil WEB_BALL reconstruido con velocidad 4 y 7
de daño, se detiene al tocar terreno y aplica inmovilización temporal al
impactar. La selección y el viento de preparación son una aproximación de la
IA/animación original; el test C cubre impacto, daño y bloqueo por pared. CTest
12/12 pasó. El VPK 01.01 compila con VitaSDK. La suite Python ejecutó 156 tests,
pero dos módulos no importaron porque WSL no tiene Pillow, pip ni venv; el
workflow ahora instala Pillow explícitamente. Los route proofs se regeneraron
con el fingerprint actual y el paquete de 113 archivos genera el replay de 545
segmentos/3,184 ticks. Vita3K 0.2.1 SDL/OpenGL termina con `0xC0000005` antes de
crear una sesión nativa; el resultado está en `_local/vita3k-vagante-test101-final/`.
ACT_ONE sigue incompleto.
Véase [ITERATION101.md](ITERATION101.md).

## Iteración 102 — emboscada del Lurker

Lurker dejó de compartir la persecución genérica. La rama nueva usa el aviso
de 16 ticks y el daño base 4–16 observados en Ghidra, golpea una sola vez por
ataque y espera antes de volver a morder. Su radio de activación y los tiempos
de animación/recuperación siguen aproximados porque el paquete de salas no
contiene el sensor ni la metadata del sprite de retractarse. CTest pasa 12/12 y
la suite Python 170/170; VPK 01.02 compila con VitaSDK. Los proofs de los tres
pisos y el replay de 545 segmentos validan. Vita3K agotó 30 s antes de crear una
sesión nativa; el resultado está en `_local/vita3k-vagante-test102/`. ACT_ONE
continúa incompleto.
Véase [ITERATION102.md](ITERATION102.md).

## Iteración 110 — integrar daño base del jugador

El análisis Ghidra de `Player::updateStats` calcula `meleeDamage` y
`rangeDamage`; `Player::attack` pasa el daño del jugador a `Weapon::startSwing`,
y `Weapon::doCollision` suma `wielderDamage` al rango propio de la forma antes
de aplicar multiplicadores. El runtime ahora conecta `DerivedStats.melee_base`
al estado de ataque cuerpo a cuerpo y `ranged_base` al daño al lanzar flechas.
Las formas melee todavía usan el punto medio de su rango y el daño del jugador
se aplica como valor esperado; falta recuperar tiradas aleatorias, bonos de
equipo/perks, críticos y multiplicadores de colisión. El test C de melee cubre
la suma del bono; VitaSDK pasó comprobación estricta de sintaxis para las fuentes
afectadas, Python 171/171 y los 12 tests C host pasan 12/12 con el runner Windows.
Las rutas de los tres pisos pasan con proofs v3 actuales (1,741/736/375 ticks);
la replay de campaña tiene 545 segmentos/3,184 ticks y el bundle valida sus 113
archivos. El VPK 01.10 se construyó y verificó como ZIP/SFO. Las dos pruebas
Vita3K (intro y campaña) expiraron antes de sesión nativa. También fallaron
pruebas intro en las builds locales 0.2.1.4170, 4183 y 4190 (OpenGL/SDL o
consola); el perfil de 4190 fue descargado de Windows Nightlies y quedó aislado
en `_local`. CTest no se ejecutó.
ACT_ONE sigue incompleto. Véase [ITERATION110.md](ITERATION110.md).

## Iteración 109 — localizar bloqueo de Slime en Vita3K

La sesión nativa 107 identifica `slime_step` de la entidad 18, tick 435 del
tercer piso, como el punto donde deja de avanzar. La llamada no retorna y falta
aislar el subpaso. El movimiento ahora protege la conversión de velocidad a
contador de iteraciones frente a valores inválidos; se añadió cobertura C aún
pendiente de ejecutar. VitaSDK Windows compiló VPK 01.09 (835,860 bytes) con
SFO `APP_VER=01.09`, alineado a `vita/iteration.cmake`. El smoke compara el
formato 1.09 de Vita3K y una prueba de regresión cubre iteraciones 100+. La
suite Python pasó 171/171. El intento Vita3K 01.08 no creó sesión nativa y dejó
un diálogo de error de memoria. La campaña 01.09 también falló antes de la
sesión nativa, con log vacío y sin código de salida; su resultado queda en
`_local/vita3k-vagante-test105-long/runs/iteration109/run-000001/`. La versión
01.09 todavía no se ha validado en el emulador.
WSL responde `E_ACCESSDENIED`, por lo que CTest no corrió. ACT_ONE sigue
incompleto. Véase [ITERATION109.md](ITERATION109.md).
## Diagnóstico actual — Dragon del primer piso

El fixture host separado ya completa una ruta repetible en el piso 0 con el
Dragon de prueba en el ancla `[20,29]`: lo derrota tras siete impactos y llega
a la salida con 30 HP en 2,283 ticks. El runner de campaña pasó también el piso
1 y el piso 2 con Goblin King. Esto acredita el simulador de rutas para ese
fixture; el Dragon sigue fuera del bundle de entrega mientras falten selección
de spawn/RNG y comportamiento nativos completos.

Vita3K 0.2.1.4194 desde `C:\Vita3k` vuelve a agotar el timeout antes de crear
una sesión nativa, así que aún no hay resultado de juego en el emulador. Worm,
la máscara final de jefes y paridad completa de Dragon también siguen
pendientes. Véase [DRAGON_ROUTE_DIAGNOSTIC163.md](DRAGON_ROUTE_DIAGNOSTIC163.md).

## Actualización 01.66 — estadísticas del Worm

El runtime del Worm ahora usa 100 HP, contacto aleatorio de 2–4 y cooldown de 30
frames según la evidencia Ghidra. Las 13 suites C host y las tres rutas actuales
de ACT_ONE pasan; en el primer piso el Worm cae y el jugador conserva 36 HP. El
VPK 01.66 compila. El smoke de campaña se preparó desde el emulador en
`C:\Vita3k` con filesystem aislado y preflight correcto, pero Vita3K venció el
timeout de 45 segundos sin crear sesión nativa. No cuenta como validación del
juego en el emulador. Véase [ITERATION166.md](ITERATION166.md).

El reintento Qt/OpenGL visible de 01.66 mostró un fallo de arranque de Vita3K
antes del título: violación de acceso por lectura nula (`ucrtbase.dll+0x25F31`,
llamada desde `Vita3K.exe+0x683965`). El mismo fallo sucede al iniciar sin el
juego y sin opciones de configuración del harness; el VPK no alcanza a
cargarse. Para el detalle véase [WORM_BUILD_DIAGNOSTIC.md](WORM_BUILD_DIAGNOSTIC.md).

## Estado de validación — 01.96

Las 13 suites C host, 220 pruebas Python y 70 subpruebas pasan. El simulador
produce proofs de ruta para los tres pisos con Rogue. En Vita3K 0.2.1.4194,
aislado desde `C:\Vita3k`, Knight completó el piso 0, derrotó al jefe y cargó el
piso 1. La prueba Rogue no creó sesión nativa; el intento visible dejó un
diálogo de error de Vita3K cuyo detalle aún no se pudo leer. La campaña completa
de tres pisos, la entrada a la cueva con Rogue y hardware Vita siguen sin
verificación. La compilación 01.96 es diagnóstica y no es de entrega; ACT_ONE
sigue incompleto. Véase [ITERATION196.md](ITERATION196.md).

## Build limpia — 01.97

Se quitó la traza temporal y se generó el VPK 01.97 con VitaSDK Windows, ya que
WSL sigue devolviendo `E_ACCESSDENIED`. La build, las 13 suites C host, las 222
pruebas Python y la estructura del VPK pasan. Los proofs V4 verifican los
paquetes VWI2, VWE2 y VWE3 por piso; el generador de replay ya consume esas
trazas y rechaza cambios en VWE3. La campaña con Worm provisional en posición
tardía pasa el auditor y las tres rutas Rogue en host, aunque cada piso se
simuló por separado y esa posición no representa spawn nativo recuperado.
También se lanzó un smoke de campaña completa desde el perfil aislado creado
con `C:\Vita3k`: Vita3K 0.2.1.4194 quedó inactivo 300 segundos sin crear sesión
nativa y los logs quedaron vacíos. Por tanto, la campaña de Vita3K sigue sin
validar. También quedó preparado el replay de entrada desde la cueva hasta la
salida del primer piso (224 pasos), aunque aún no se ejecutó en una sesión
nativa. Una ejecución depurada de Vita3K con `--skip-title` reprodujo una
violación de acceso antes de cargar cualquier juego (`ucrtbase.dll+0x25F31`,
`Vita3K.exe+0x683965`), por lo que el bloqueo actual pertenece al arranque del
emulador y no aporta evidencia sobre el VPK. Véanse [ITERATION197.md](ITERATION197.md)
y [WORM_BUILD_DIAGNOSTIC.md](WORM_BUILD_DIAGNOSTIC.md).

## Auditoría de transición — tres pisos

La cadena host ahora reproduce las trazas VTR1 con los paquetes reales y
conserva HP, atributos e inventario mediante la misma función de transición
del runtime. La corrida uniforme Rogue vence los tres pisos por separado, pero
al encadenarlos llega al Worm con 10/80 HP y muere en el tick 870; el piso 1
aislado terminaba con 2 HP tras comenzar lleno. Las pruebas anteriores mezclan
además Rogue en los pisos 0–1 y Knight en el 2, mientras que el replay normal
elige Knight por defecto. Por ello, los tres proofs separados no acreditan una
campaña jugable. Falta una ruta/combate que sobreviva con estado persistente;
no se añadirá curación entre pisos hasta verificarla en el original. El
diagnóstico está en [ITERATION197.md](ITERATION197.md) y se repite con
`tools/test_act_one_campaign_chain_windows.ps1`.

## Demo local — 01.98

Se generó `_local/demo-act-one-198/` con VPK `VAGV00006` 01.98 y el paquete local de intro/ACT_ONE para `ux0:data/Vagante2Vita`. El VPK pasa el verificador de la skill Vita-port (7 entradas, sin recursos comerciales); pasan las 13 suites C host, 222 pruebas Python y 70 subpruebas. El replay determinista intro→piso 0 se prepara con 224 segmentos, 37 ticks hasta la cueva y 1,268 ticks de ruta del piso 0. Las tres rutas de piso pasan por separado con Rogue en el simulador host.

La cadena Rogue con salud persistente falla: completa el piso 0 con 10/80 HP y muere en el piso 1 a tick 918 antes de dañar al Worm. La prueba de entrada por la cueva para VPK 01.98, Vita3K 0.2.1.4194, runtime/perfil aislados desde `C:\Vita3k` y OpenGL agotó 45 segundos sin sesión nativa ni logs. Es una limitación del arranque de Vita3K observada en este entorno; no se declara validado el gameplay del VPK. La demo y su instalación están descritas en [DEMO_ACT_ONE_198.md](DEMO_ACT_ONE_198.md).

## Demo local — 01.99, resolución alta

La iteración 01.99 deja 640×360 como resolución lógica inicial y la escala a 960×540, dentro del framebuffer de 960×544. En los menús, L conserva el modo ligero de 320×180. No se agregan buffers. El VPK contiene siete entradas y pasa `check_vpk.py`; pasan las 13 suites C host, 222 pruebas Python y 70 subpruebas. Se preparó `_local/demo-act-one-199/` con 189 archivos inventariados y recursos externos ACT_ONE validados (144 archivos).

La prueba de intro→cueva del VPK 01.99 en Vita3K 0.2.1.4194 volvió a agotar 45 segundos antes de crear una sesión nativa, aun con OpenGL y una copia aislada completa del emulador y perfil. No confirma la calidad visual en Vita3K ni en hardware. Instrucciones y evidencia: [DEMO_ACT_ONE_199.md](DEMO_ACT_ONE_199.md) y `_local/vita3k-c197-fullcopy-test/runs/iteration199/run-000001/`.

## Iteración 200 — verificación del Worm

El probe host ahora prioriza melee del Rogue cuando el Worm o una amenaza
cercana está al alcance; el arco gastaba flechas contra terreno desde el
saliente. La corrida `_local/route-act-one-199-close-melee-fullchain/` pasa los
proofs físicos individuales de los tres pisos. Eso todavía no representa una
cadena con salud persistente: el piso 0 termina con 10 HP en la corrida actual
(20 HP en una ruta anterior), y el piso 1 con el mejor estado conocido de
20 HP/26 flechas termina en muerte al tick 934. La ejecución encadenada actual
entra con 10 HP al piso 1 y muere al tick 850. Las 14 suites C host y 226
pruebas Python pasan. No se simula curación entre pisos; hacen falta datos del
juego original y una ruta encadenada que sobreviva. Vita3K sigue fallando antes
de cargar el VPK. Véase [ITERATION200.md](ITERATION200.md).

## Iteración 201 — golpe al jugador

Las exportaciones Ghidra locales recuperan `Player::hurt` (`0x006f9020`): tras
un daño aceptado pone `hurtTimer=10`, libera el agarre y calcula knockback según
atributos y estado. `Player::doCollision` consulta `invulFrames` por entidad;
esto no confirma inmunidad global y no evita por sí solo contactos de fuentes
distintas. No se integró una reacción parcial sin recuperar `Unit::hurt` y los
argumentos exactos de knockback. Los proofs físicos individuales siguen pasando,
pero el estado persistente de 10 HP falla al tick 850 del piso 1; Vita3K sigue
sin cargar el VPK. Véase [ITERATION201.md](ITERATION201.md) y la evidencia local
en `_local/decompiled/006f9020.c`, `006f74c0.c` y `006fc9f0.c`.

## Iteración 202 — salida de pantalla completa

La revisión del escalador encontró filas que se pisaban al mapear 640×360 a
960×540. El mapeo corregido cubre cada píxel visible de Vita y conserva el
perfil ligero; las 14 suites C host y 226 pruebas Python pasan. El VPK 02.02
se compiló con VitaSDK Windows, verifica sus siete entradas y mantiene los
recursos de ACT_ONE en el paquete local de 189 archivos. Vita3K falla antes de
cargar un título con `0xC0000005`; WSL no inició por `E_ACCESSDENIED`. La cadena
de salud persistente y la ejecución en hardware siguen pendientes. Véase
[ITERATION202.md](ITERATION202.md) y [demo local](DEMO_ACT_ONE_202.md).

## Iteración 203 — feedback visual de daño

Ghidra confirma que `Player::hurt` inicia `hurtTimer=10` al aceptar daño. El
runtime incorpora un destello rojo de 10 ticks en las cuatro rutas de impacto
que ya aplican HP, sin alterar daño, desplazamiento ni inmunidad. Pasan las 14
suites C y 226 pruebas Python; VitaSDK compila el VPK 02.03 y el verificador
reporta siete entradas sin errores. El bundle local vuelve a validar sus 189
archivos por SHA-256. Vita3K continúa fallando antes de cargar títulos y la
cadena de campaña persistente no completa el piso 1. Véase
[ITERATION203.md](ITERATION203.md) y [demo local](DEMO_ACT_ONE_203.md).

La auditoría de cadena de 02.03 regeneró rutas Rogue separadas, pero sus proofs
inician en 80/80 HP y 30 flechas cada uno: floor 0 termina en 10 HP, floor 1
termina en 4 HP y floor 2 en 45 HP. La ejecución integrada lleva correctamente
10 HP/23 flechas al Worm; una corrida histórica sin guard murió al tick 850 tras
tres contactos en 37 ticks. El gate integrado ahora detecta y rechaza el proof
de floor 1 antes de simularlo porque su estado inicial no corresponde al
jugador transportado. La ruta de evasión experimental no mejoró el resultado y
se retiró. El primer ACT_ONE aún no está validado como campaña persistente ni
en Vita3K.

## Estado actualizado — 20 de octubre de 2026

La iteración publicada localmente sigue siendo el VPK 02.03 (`VAGV00006`). En
esta revisión se repitieron las 14 pruebas C host y las 226 pruebas Python:
ambas suites pasan. El VPK contiene siete entradas y pasa `check_vpk.py` sin
errores ni advertencias; el bundle privado local contiene 189 archivos y todos
coinciden con el manifiesto SHA-256. SHA-256 del VPK:
`B598F7C865E127636255FCFBCF50C563A33BB33C939D9AA08D9A509D9161D221`.

La cadena no está completa: proofs independientes parten cada uno con 80 HP y
30 flechas, mientras que la cadena transporta 10 HP y 23 flechas al primer
Worm. El validador integrado detecta ese estado incompatible antes de aceptar
el proof aislado. El selector de staging experimental del probe tampoco ha
demostrado una ruta válida hasta el jefe. No se atribuye progreso de campaña a
esas rutas individuales. La ejecución actual de Vita3K sigue sin validar el
gameplay; el diagnóstico anterior termina con `0xC0000005` antes de cargar un
título. El build tampoco se ha probado en una Vita física.

El repositorio público conserva código, herramientas, pruebas y análisis
reproducible. Los sprites, mapas y paquetes exportados desde la copia local de
Vagante permanecen fuera de Git; cada persona debe generar sus recursos locales
con su propia copia. Véanse [ITERATION203.md](ITERATION203.md),
[DEMO_ACT_ONE_203.md](DEMO_ACT_ONE_203.md) y el [inventario local](LOCAL_ARTIFACTS.md).
