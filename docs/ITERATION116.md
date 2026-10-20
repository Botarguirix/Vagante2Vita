# Iteración 116: verificar el comparador nativo de objetivos

Se aisló como helper `goal_candidate_rank` la prioridad usada al ordenar los
candidatos de meta. Se contrastó contra el comparador Ghidra de
`LevelGenerator::addStartAndGoal` (`0x00689c60`): más lados abiertos primero,
apoyo sólido primero, tile que no sea plataforma primero y mayor coordenada Y
primero. La prueba transcribe el predicado nativo y comprueba todas las parejas
de tiles en una matriz con sólidos, plataformas, vacíos y agua. Esto valida la
relación de orden; no pretende reproducir el orden interno de MSVC
`std::sort` para elementos equivalentes.

Con la prioridad ahora documentada y probada, la brecha concreta se reduce al
ordenamiento de empates, al conjunto exacto que produce el recorrido de
candidatos y a los draws previos de enemigos/jefe. No se cambió el runtime Vita.

## Validación

- `python -m unittest discover -s tests -p test_floor_layout.py -v`: 20/20.
- Suite completa: 182/182.
- Seed 12345/piso 1 regenerada y exportada en `_local/iteration116/`. Conserva
  el paquete de terreno VWI2 (80×40; SHA-256
  `33364658a0547dd2583281b2d3b0609d9d00e2c116f47d0b75d019815171a6b4`), 29
  entidades y 7 enemigos provisionales. El reporte correctamente mantiene
  `floor_complete=false` y `exact_generation_parity=false`.
- No cambió el código C ni el VPK; no se ejecutó Vita3K.

## Siguiente paso

Recuperar la implementación exacta de `std::_Sort_unchecked`/su versión MSVC y
las trazas de consumo que llegan a selección de extremos y `addBoss`. Luego se
debe integrar la ruta real de jefe y verificarla en el runtime antes de afirmar
que el mundo está terminado.
