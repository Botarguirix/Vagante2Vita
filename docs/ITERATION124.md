# Iteración 124: tipo `T` de `addBoss` es Thief

La identificación anterior de `EntityType` 84 como Shrine era incorrecta.
El mapa nativo de `EntityType` asigna 84 a `TYPE_THIEF`; Shrine tiene otro
valor. Ghidra muestra `Thief::Thief` (`0x007c8370`) llamando a
`Monster::Monster` con un cuerpo de 7×12. Como `addBoss` mide el centro con la
mitad del cuerpo, el filtro usa ese tamaño al convertir su posición a tile.
Shrine ya no aparece como bloqueador `T`.

La aparición aleatoria del Thief no forma parte de los rolls ordinarios de
enemigos. La revisión de la rama confirma que el `4` visto antes era una
comparación de `ActType`, no un enum de sala. Al inicio de
`addRandomEnemies`, el juego consume un entero; si `roll % 5 == 0` y
`floorIndex > 0`, habilita un presupuesto local de un Thief para ese nivel.
La función después exige separación mayor a 14 tiles, ruta
`tilePathsToEdgeOfRoom` y presupuesto disponible; al crear al ladrón consume
otro entero para el offset horizontal `% 16` y aumenta `thievesSpawned`. PDB lo sitúa en
`LevelGenerator` offset 40 y `GameSave::LevelGenDat` offset 16; el contador se
copia entre el generador y el save. Su tope efectivo y la relación exacta entre
esa sala y la entrada todavía no están mapeados con suficiente certeza para
generar instancias exactas.

`provisional_enemies` ahora conserva el primer draw real y registra el gate y
su presupuesto (cero en floor 0; hasta uno en floors 1 y 2), pero no consume el
draw de posición ni inventa un Thief. `boss_room_spawn_geometry`
acepta posiciones dinámicas de Thief en tiles para evaluar el filtro cuando un
caller pueda aportarlas. Así se separa el filtro de geometría de la generación
incompleta.

## Validación y alcance

- Las pruebas cubren E/G/K como bloqueadores estáticos, Shrine como control,
  Thief=84 como entidad dinámica en tile-space, los límites del gate 1/5 y que
  la posición dinámica no se invente.
- Suite focalizada y suite Python completa pendientes de repetirse tras esta
  corrección.
- `native_spawn_ready` sigue en falso. Tope del contador persistente,
  asociación de la sala de entrada, orden de candidatos y estado exacto del RNG antes de `addBoss`
  siguen sin reconstruir.
- No se regeneró el VPK: el cambio corrige el análisis offline de geometría.
