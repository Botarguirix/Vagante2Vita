# Iteración 114: traza provisional del punto de llamada del jefe

`addRandomEnemies` (`0x0068d1e0`) recorre las salas en el orden barajado y llama
a `addBoss` en el turno de la sala elegida, después de consultar
`Room::monstersAllowed`. Antes de esa llamada, las salas normales pueden
barajar sus tiles y consumir rolls de spawn. Dentro de `addBoss`, el fallback
solo consume sorteos si no hay ningún ancla geométrica: genera coordenadas X/Y
hasta reunir cuatro tiles abiertos, conserva duplicados y baraja el vector.
Después se consumen los draws para seleccionar el tipo/slot del jefe.

La reconstrucción de la lista de salas ahora conserva además un filtro nativo
confirmado: con `Room::ActType::ACT_ONE == 0`, el branch en
`0x0068d799–0x0068d7ab` añade salas solo cuando `y != 0`. Por tanto la fila
superior no participa en el shuffle de salas de enemigos. Como la única
llamada xrefeada a `addBoss` está dentro de ese recorrido, una sala de jefe en
`y=0` tampoco llega a esa llamada. Se conserva este comportamiento observado
en el modelo; no se fuerza la aparición de un jefe que el flujo no invoca.

El layout ahora incluye `provisional_enemy_rng_trace`, una auditoría del orden
simulado, draws por shuffle y roll, y el índice donde el flujo nativo debería
invocar `addBoss`. No ejecuta la llamada ni los draws del jefe. La traza está
marcada `native_parity=false`: los filtros de entidades, tiles y presupuestos
del modelo provisional aún no reproducen todas las condiciones nativas, y la
reconstrucción de RNG de entidades de sala anterior sigue incompleta. Por eso
el índice ayuda a localizar el desfase, pero no es el estado MT19937 nativo que
se necesita para resolver el fallback.

Las pruebas unitarias verifican tanto la posición de `addBoss` en el recorrido
como la exclusión de una sala de jefe en `y=0`. El barrido de 60 layouts de tres
pisos valida la traza integrada y ambas ramas de ubicación de jefe.

## Validación

- Suite Python completa: 180/180.
- Pruebas enfocadas tras recuperar el filtro `y != 0`: 19/19 para el modelo
  de layout y 2/2 para el barrido ACT_ONE de 60 layouts.
- Seed 12345, piso 1, se regeneró y exportó en `_local/iteration114/`:
  VWI2 80×40 (SHA-256 `33364658a0547dd2583281b2d3b0609d9d00e2c116f47d0b75d019815171a6b4`),
  VWE2 con 29 entidades y VWE3 con 7 enemigos provisionales. La exportación
  conserva `floor_complete=false` y `exact_generation_parity=false`.
- No hubo cambios en el runtime C; el VPK 01.11 existente no contiene cambios
  de esta iteración.
- WSL sigue sin poder enumerar distribuciones (`Wsl/EnumerateDistros/Service/
  E_ACCESSDENIED`). No se ejecutó Vita3K en esta iteración.

## Siguiente paso

Recuperar los filtros y consumos condicionales de `addRandomEnemies` desde la
lista de instrucciones y constructores de entidades; luego ejecutar
`addBoss` en su posición de sala con un estado RNG respaldado por una traza
completa. El jefe aún no está serializado en VWE3 ni integrado en el runtime,
así que ACT_ONE no está completo.
