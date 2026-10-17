# Iteración 059: pesos estáticos de las categorías de cofre

Se amplió `tools/audit_chest_pool_graph.py` para seguir las cuatro llamadas a
`ItemPoolRarity` en el listing del constructor, leer los floats del ejecutable
PE y contrastar los IDs con el enum PDB. La salida local
`_local/chest-pool-graph.json` confirma el reparto de `POOL_CHEST`:

| Categoría | ID | Peso |
| --- | ---: | ---: |
| Armas | 15 | 30 |
| Armadura | 6 | 60 |
| Otros | 22 | 4 |
| Varitas | 11 | 6 |

Los pesos suman 100 y se leen en VA `0x00b204fc`, `0x00b205a4`, `0x00b203b4`
y `0x00b203fc`, respectivamente. La pseudodecompilación conserva expresiones
temporales, pero el listing y el ejecutable permiten recuperar los valores
estáticos sin inferirlos.

## Verificación y alcance pendiente

- Extracción real desde listing, PE, pseudodecompilación y PDB:
  `verified_structure=true`, `verified_weights=true`.
- Suite Python: 128 pruebas aprobadas; se prueba la asociación de IDs y
  direcciones de peso.
- Aún falta reconstruir las tablas internas de armas/armadura/otros/varitas,
  aplicar nivel y exclusión de objetos únicos, y crear/dibujar/recoger el Item
  en el mundo. No hubo cambios al VPK 057 ni prueba nueva de Vita3K.
