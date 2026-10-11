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
