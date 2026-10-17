# Estado de reconstrucción (17 de octubre de 2026)

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

## Estado actual: iteración 090

La desensamblación de `LevelGenerator::addRandomEnemies` recuperó el orden
x-major y el uso de `std::shuffle` para salas y celdas candidatas, además del
roll uniforme de 31 bits. El generador Python ahora aplica esas etapas antes
de los filtros aproximados y mantiene el filtro geométrico de Lurker. El
bundle 090 conserva 8, 8 y 20 spawns provisionales; los VWI2/VWE2 aún coinciden
con proofs de ruta seed 12345. Pasan 167 pruebas Python; las pruebas C y rutas
físicas previas siguen documentadas en iteración 086.

El paquete privado 090 valida 108 archivos, tres pisos y 56 imágenes, pero
mantiene `campaign_complete=false`: los filtros nativos de tiles/entidades y
el RNG intercalado no están completos. El VPK 00.90 se compiló con VitaSDK
Windows (830,423 bytes; SHA-256
`9A42FCC106F769523AD7B6F5C0AF7ECC9D921CD2518A92C8DDAF21E53F0204A9`). El
preflight preparó una replay de campaña de 545 segmentos/3,184 ticks. Vita3K
0.2.1.4183 agotó 90 segundos antes de crear sesión nativa o logs. La
automatización de escritorio no está conectada en esta sesión. Siguen
pendientes paridad completa, jefe, transiciones y validación de campaña en
Vita3K/hardware. Véase [ITERATION90.md](ITERATION90.md),
[ITERATION89.md](ITERATION89.md) y
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
