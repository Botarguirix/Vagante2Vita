# Iteración 125: gate RNG del Thief ACT_ONE

La iteración 124 describió incorrectamente una “sala enum 4” para la rama del
Thief. El argumento en `addRandomEnemies` es `ActType`; ese `4` pertenece a la
ruta ACT_FOUR, no a un `room_type`. Esta iteración corrige esa lectura.

Al entrar a `LevelGenerator::addRandomEnemies` (`0x0068d1e0`), el juego consume
un entero uniforme. El listing muestra que si `roll % 5 == 0`, el presupuesto
local se asigna a `floorIndex != 0`. Por tanto ACT_ONE floor 0 nunca habilita
esta aparición; floors 1 y 2 pueden habilitar un único Thief por ejecución.
El presupuesto local se reduce al colocar el ladrón, y el contador persistente
`thievesSpawned` también aumenta. La colocación consume luego otro entero para
un offset horizontal `% 16`; aún faltan la ruta exacta de candidatos, su
asociación con la entrada y la coordenada final del spawn.

`provisional_enemies` ahora conserva el primer entero ya consumido y registra
el resultado del gate, sin añadir un Thief ni consumir el draw de posición.
`boss_room_spawn_geometry` acepta ubicaciones de Thief dinámicas en tile-space
para que una generación futura pueda aplicar el filtro `T=84`.

## Validación

- Suite Python: 191/191.
- Barrido de 100 seeds por piso: piso 0, 0 gates activos; piso 1, 23/100; piso
  2, 21/100. Los conteos concuerdan con el roll registrado; no son pruebas de
  que se haya colocado un Thief, pues el filtro geométrico y el punto de spawn
  no están simulados.
- Pruebas añadidas verifican límites del gate y presencia de casos habilitados
  en el barrido existente de 20 seeds por cada piso ACT_ONE.
- No se reconstruyó el VPK: el cambio actualiza el simulador de generación y
  su reporte offline.

`native_spawn_ready` sigue en falso. La siguiente recuperación debe enlazar el
`entranceLocation`, seguir el orden y filtros de celdas de `addRandomEnemies`,
y reproducir el punto de aparición antes de introducir Thief en el paquete VWE3.
