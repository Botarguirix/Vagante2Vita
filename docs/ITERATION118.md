# Iteración 118: rama de enemigos acuáticos y guía de Decompedia

Se integró al trace provisional de enemigos la rama de agua observada en
`LevelGenerator::addRandomEnemies` (`0x0068d1e0`). La condición consulta el tile
tipo 9 y la masa de fluido en `fluid[2 * (x + fluidWidth * y)]`; con el ancho de
fluido de `2 * level.w`, el acceso corresponde al sub-tile superior izquierdo
de 8 px (`(2*x, 2*y)`). Una llamada a `generate_canonical<float, 4294967295>`
selecciona Piranha (`TYPE_PIRANHA=16`) bajo 0.8 y Minnow (`TYPE_MINNOW=17`) desde
0.8.

El mapa `Monster::MONSTER_SPAWN_WEIGHT` del inicializador `0x004035f0` confirma
pesos 0.5 para Piranha y `0x3e2aaaab` (float32 aproximado a 1/3) para Minnow.
El simulador usa esos valores al calcular el presupuesto provisional de sala y
registra tile, masa, draw y subtipo. Las pruebas verifican índice, umbral,
pesos y consumo RNG. Sin embargo, los filtros que conducen al branch siguen
siendo aproximados, los peces son solo trace y no se serializan a VWE3; por
tanto no se afirma paridad de spawn ni se resuelve el soporte de runtime.

## Revisión de Decompedia

La portada de [Decompedia](https://decomp.wiki/) reúne recursos como
[decomp.dev](https://decomp.dev/) para informes de progreso,
[decomp.me](https://decomp.me/) para matching colaborativo y una instancia
compartida de Ghidra. Su distinción entre *matching decompilation* y
reconstrucción funcional confirma que VaganteVita sigue el segundo camino:
recuperamos comportamiento del ejecutable Win32 y lo reimplementamos para
ARM/Vita; no perseguimos una recompilación idéntica del binario. La wiki sirve
para descubrir métodos y herramientas, pero no aporta una herramienta directa
para convertir el pseudocódigo Ghidra de Vagante en código portable.

Se mantiene el análisis Ghidra local porque la base usa EXE/PDB y contenido
privados del usuario. No se carga nada en decomp.me ni en el servidor público.
Las páginas de matching, comparadores de assembly y permuters solo serían
relevantes si se fija una función, arquitectura y toolchain original para
igualar bytes; no son criterios adecuados para verificar esta implementación
Vita. La guía útil que sí aplicamos es registrar procedencia de cada hallazgo,
separar observación nativa de hipótesis y medir cada sistema en su propio
entorno de ejecución.

## Validación

- `test_floor_layout.py`: 25 pruebas enfocadas pasan.
- Barrido ACT_ONE de 20 seeds × 3 pisos: 3 pruebas pasan.
- Suite completa: 187 pruebas pasan.
- `git diff --check`: pasa; solo muestra avisos de normalización CRLF/LF en
  archivos preexistentes.
- Seed 12345/piso 1 exportada de forma privada en
  `_local/iteration118/floor1-export/`. VWI2 conserva SHA-256
  `33364658a0547dd2583281b2d3b0609d9d00e2c116f47d0b75d019815171a6b4`.
- El piso sigue `floor_complete=false`; no se recompiló VPK ni se validó en
  Vita3K. Los peces siguen fuera del paquete/runtime.
