# Iteración 113: geometría del spawn de jefe

Se implementó `boss_room_spawn_geometry` en `tools/floor_layout.py`, apoyado en
el flood fill existente `tile_paths_to_room_edge`. Sigue el patrón de
`LevelGenerator::addBoss` (`0x0068a770`): recorre el cuarto seleccionado en
orden x-major/y-major, requiere caminos por tiles no sólidos desde los cuatro
tiles del cuerpo 2×2 hasta un borde enlazado y exige dos tiles de apoyo sólidos
debajo. También reconoce los bloqueadores estáticos confirmados por el PDB:
`E=69` Spikes, `G=71` BoulderTrap, `K=75` SpikeTrap y `T=84` Shrine. Calcula
su tile central con el cuerpo de 16×16 y elimina anclas donde coincide con la
fila inferior del jefe. El reporte diferencia candidatos geométricos y los
que sobreviven a esas entidades de sala. El filtro no cubre entidades creadas
dinámicamente. `boss_spawn_random_fallback` reconstruye la rama que reúne cuatro
posiciones abiertas mediante sorteos x/y, conserva duplicados y baraja la lista.
Necesita el estado MT19937 exacto al entrar en `addBoss`; como el pipeline aún
no recupera todos los sorteos previos, no aplica el fallback al reporte y
`native_spawn_ready` sigue falso.

Para seed 12345, se observan 4, 3 y 11 candidatos geométricos en floors 0–2;
tras el filtro estático sobreviven 4, 0 y 11. Floor 1 muestra por qué hace
falta conectar el fallback modelado con el estado nativo antes de elegir un
spawn. Ningún superviviente
equivale aún al punto final nativo del jefe.

## Validación

- `python -m unittest discover -s tests -v`: 178/178.
- Las pruebas unitarias verifican footprint 2×2, soporte, conectividad hasta
  borde enlazado, filtrado por `TYPE_SPIKES`, fallback con duplicados y el
  guard de seguridad cuando no hay tiles abiertos.
- El barrido de 20 semillas por cada uno de tres pisos verifica forma e
  invariantes del nuevo campo para 60 layouts privados.
- No cambió código C de Vita, no se generó VPK y no se ejecutó Vita3K.

## Pendiente

Completar fuentes dinámicas del filtro y recuperar el estado MT19937 de entrada
para aplicar el fallback/shuffle al layout real. Luego habrá
que serializar un tipo de jefe y recuperar IA, daño, muerte y transición antes
de acreditar un piso ACT_ONE jugable.
