# Iteración 060: pesos del grafo anidado de objetos

El auditor ahora asocia cada vector `ItemPoolRarity` con la clave de mapa
insertada inmediatamente después, resuelve los IDs con el enum PDB y carga
cada peso de su VA estática en el PE. El resultado local completo está en
`_local/chest-pool-graph.json`.

Se verificaron siete grafos: `POOL_CHEST` (4 aristas), `POOL_POTIONS` (2),
`POOL_WEAPONS` (7), `POOL_ARMOR` (6), `POOL_EQUIPMENT` (2), `POOL_ALL` (5) y
`POOL_SHOP` (6). En particular, el camino de cofres queda demostrado como
`POOL_CHEST` → `POOL_WEAPONS`/`POOL_ARMOR`/`POOL_OTHER`/`POOL_WANDS`, con pesos
30/60/4/6, y las ramas de armas y armadura ya tienen sus pesos de subcategoría.

## Alcance y verificación

- El extractor verifica los nombres PDB, direcciones y pesos finitos positivos;
  las categorías del cofre suman 100.
- La suite Python existente pasó 128 pruebas al comenzar este trabajo; hay que
  volver a correrla tras cerrar la iteración.
- El selector nativo todavía debe filtrar `ItemRarity` por nivel, aplicar la
  exclusión de únicos, elegir la fila ponderada y crear/dibujar/recoger el Item.
  No se ha modificado el runtime ni la VPK.
