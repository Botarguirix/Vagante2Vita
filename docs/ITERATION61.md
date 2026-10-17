# Iteración 061: datos de botín cerrados y empaquetados

Se corrigió `tools/extract_item_pools.py` para reconocer la asignación del pool
que Ghidra representa a través del mapa temporal `local_1008`. La tabla real
ahora contiene 19 pools hoja y 170 filas: 16 tipos en `POOL_POTIONS_GOOD` y 10
en `POOL_POTIONS_BAD`, además de las 18 tablas restantes.

`tools/compile_item_loot.py` consolida las hojas y el grafo anidado en
`_local/item-loot.vlt`, inicialmente un paquete `VLT1` de 2.402 bytes: 19 pools hoja, 170
filas, siete pools anidados y 32 aristas ponderadas. El compilador valida todas
las referencias y recorre el grafo del cofre para comprobar que existe un
objeto elegible en cada nivel 0–10. El nivel 0 no ofrece filas en
`POOL_FIST`; Ghidra muestra que el selector descarta una subrama que devuelve
`ITEMTYPE_UNKNOWN` y vuelve a ponderar las restantes.

## Verificación y pendiente

- Extracción desde PE/listing/Ghidra/PDB: 170/170 filas asignadas; salida de
  VLT1 validada contra enums y referencias.
- Suite Python: 132 pruebas aprobadas, incluidas pruebas de formato, cierre de
  referencias y fallo de cobertura por nivel.
- La iteración 062 añade sprites/únicos y cambia el paquete a VLT1 versión 2,
  ahora de 3.082 bytes; se integra la selección y recogida en el runtime. Véase
  [ITERATION62.md](ITERATION62.md). Vita3K aún no registra una sesión nativa.
