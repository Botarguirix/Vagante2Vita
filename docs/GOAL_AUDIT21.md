# Auditoría del objetivo al cerrar 021

Esta auditoría no certifica finalización. La petición ampliada incluye juego
y menús completos; no se reduce al recorrido de INTRO que ya funciona.

| Requisito | Evidencia actual | Estado |
|---|---|---|
| Mundo inicial original | Tile packet/autotiling, terreno de atlas, log físico 019 WORLD_START; 020 carga 22 entidades originales | Parcial: luces, trayectorias e interacciones faltan |
| Movimientos iniciales | Pruebas de walk/jump/duck/platform/ladder/ledge; eventos físicos HANG | Parcial: adaptación de bordes sin paridad PC completa; acciones con armas faltan |
| Colisiones | CTest flags/movimiento; recorridos físicos; snap de superficie | Implementadas en subconjunto, falta comparación completa PC |
| Estadísticas | Base siete atributos + HP; 021 integra derivadas intrínsecas y carga base | Parcial: bonos/equipo/antecedentes/afinidades/combate faltan; 021 sin prueba Vita |
| Menús completos | Título y selección base; 021 ficha consultable | Pendientes opciones, selección completa, inventario funcional, skills/guardado |
| Juego completo solicitado después | Funciones de generación localizadas y documentadas | No implementado: pisos, transiciones, enemigos, combate, objetos y progreso |
| Revisiones separadas | package_iteration.py rechaza carpeta existente, manifiesto/hashes, source.zip y logs por sesión | Conservado; cada entrega debe verificarse al empaquetar |
| Identidad de aplicación | iteration.cmake fija VAGV00006/Vagante2Vita; empaquetador valida param.sfo | Conservado |
| Recursos del juego aparte | CMake sin bundle; empaquetador exige lista de archivos propios permitidos | Conservado |
| Sin métricas internas | main sin PERF/FPS/CPU; logs de estados; métricas solo en probes host | Conservado |

No hay prueba suficiente para marcar el objetivo completo. El siguiente trabajo
funcional debe conectar acciones y efectos, no cambiar los criterios de éxito.
