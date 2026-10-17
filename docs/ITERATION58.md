# Iteración 058: auditoría estructural de POOL_CHEST

`tools/audit_chest_pool_graph.py` extrae del constructor Ghidra las cuatro
aristas de `POOL_CHEST` (enum 23): `POOL_WEAPONS`, `POOL_ARMOR`, `POOL_OTHER`
y `POOL_WANDS`. La salida privada `_local/chest-pool-graph.json` registra las
expresiones de peso observadas, la huella de la pseudodecompilación y dos
estados separados: estructura verificada y pesos verificados.

La extracción real confirma las cuatro aristas, pero los pesos son variables
temporales (`uVar86`, `piVar92`, `__return_storage_ptr___00`, `piVar95`). No
se presentan como números ni se usan para seleccionar botín. El algoritmo de
`getRandomItemTypeFromPool` también filtra filas por nivel y tipos únicos,
así que todavía hace falta recuperar los valores de runtime y las tablas
anidadas antes de conectar recompensas a cofres.

## Verificación

- Extracción real: `verified_structure=true`, cuatro categorías y
  `verified_weights=false`.
- Suite Python: 127 pruebas aprobadas, incluidas dos pruebas del grafo.
- No hubo cambios al runtime ni al VPK 057; esta iteración recupera y acota
  evidencia para evitar que una aproximación se confunda con el generador del
  juego.
