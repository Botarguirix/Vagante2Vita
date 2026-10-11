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
