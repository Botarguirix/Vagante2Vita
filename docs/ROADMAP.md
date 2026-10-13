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
