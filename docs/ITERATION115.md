# Iteración 115: cerrar el catálogo RNG de entidades de sala ACT_ONE

El ledger de entidades marcaba como no clasificadas tres rutas presentes en los
cuartos ACT_ONE: `Boulder`, `BoulderBlocker` y `Doodad` waterfall tipo 20. La
exportación Ghidra de `Room::createEntity` pasa el Boulder estático como
`BOULDER_SQUARE` (enum 1); `Boulder::Boulder` solo consume tres
`generate_canonical` en la rama `BOULDER_SHAKE` (enum 0), por lo que ese Boulder
de sala no consume RNG. `BoulderBlocker` tampoco tiene draws en constructor ni
`setLevel`. `Doodad::setTypeVars` configura `DOODAD_WATERFALL_3` sin RNG. Estas
rutas ya se registran como RNG-free; los libros sin objeto continúan en la
segunda pasada de `Level::createRoomEntities`, donde se consume su generador de
spellbook.

`full_room_entity_rng_reconstructed` ahora se calcula por layout: es verdadero
cuando cada entidad colocada queda clasificada y cada pedestal vacío se incluye
en la secuencia de libros. Se añadió una auditoría de todas las definiciones de
las 287 habitaciones del catálogo ACT_ONE, además del barrido de 60 layouts.
Esto cierra el ledger de creación de entidades colocadas, pero no certifica el
estado RNG global antes de `addBoss`: todavía faltan paridad en selección de
extremos/candidatos y filtros condicionales de `addRandomEnemies`.

## Validación

- Auditoría completa del catálogo privado ACT_ONE: 8 tipos de entidad y doodads
  1, 2, 4, 5, 14, 19, 20 y 21; cero rutas RNG de entidad sin clasificar.
- `test_floor_layout.py`: 19/19.
- `test_act_one_seed_sweep.py`: 3/3, incluidos 60 layouts de tres pisos.
- No cambió el runtime C ni el VPK 01.11. No se ejecutó Vita3K.
- WSL permanece inaccesible con `E_ACCESSDENIED`.
