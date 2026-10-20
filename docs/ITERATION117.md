# Iteración 117: reproducir la rama de ordenamiento MSVC para metas

Ghidra muestra que `std::_Sort_unchecked` (`0x006974e0`) utiliza insertion
sort cuando el vector tiene menos de 32 elementos. Se implementó esa rama con
movimientos solo cuando el comparador estricto da precedencia, por lo que los
elementos equivalentes conservan su orden de entrada igual que en el bucle
nativo. Para listas de 32 o más entradas el pipeline mantiene el sort estable
de Python, pero ahora lo marca explícitamente como no exacto.

El barrido de 20 seeds × 3 pisos produjo como máximo 27 candidatos alcanzables
para la meta y como máximo 12 para el fallback. Los 60 layouts quedan por
debajo del umbral nativo y reportan `algorithm=msvc_insertion_sort` y
`exact_tie_order=true`. Esto resuelve el desempate del sort para el rango
observado; los candidatos todavía son generados con estado RNG provisional y
la lista no acredita paridad de generación.

## Validación

- `test_floor_layout.py`: 21/21.
- `test_act_one_seed_sweep.py`: 3/3; incluye el barrido de 60 layouts y el
  límite de 32 candidatos.
- Seed 12345/piso 1 regenerada y exportada en `_local/iteration117/`; el
  terreno VWI2 80×40 conserva SHA-256
  `33364658a0547dd2583281b2d3b0609d9d00e2c116f47d0b75d019815171a6b4`, con 29
  entidades y 7 enemigos provisionales. El exportador conserva
  `exact_generation_parity=false` y `floor_complete=false`.
- No se cambió el runtime C ni el VPK; no se ejecutó Vita3K.

## Pendiente

El ordenamiento de listas de 32 o más candidatos todavía usa un fallback no
equivalente al MSVC del ejecutable. La paridad de la lista de candidatos y el
estado RNG global siguen incompletos; especialmente, los filtros de
`addRandomEnemies` y el punto de entrada real a `addBoss` aún necesitan la
traza nativa completa.
