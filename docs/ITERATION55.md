# Iteración 055: extracción verificable de pools de botín

`tools/extract_item_pools.py` recupera las 170 filas `ItemRarity` del
constructor de `ItemGenerator` desde la listing de Ghidra y los cuatro enteros
del ejecutable cuando el constructor usa una fila estática. Asigna cada fila a
su pool siguiendo el pseudocódigo del constructor y los enums exportados. La
salida privada actual contiene 18 pools directos; se escribe en `_local/` y no
se añade al VPK. El auditor también resuelve el ID 62 como `ITEMTYPE_FLAIL`.

## Verificación

- `python -m unittest discover -s tests -p 'test_*.py'`: 123 pruebas aprobadas.
- Extracción con la copia local del ejecutable, listing, pseudocódigo y enums:
  170/170 filas asignadas, sin errores; club pool identifica `FLAIL`.
- `tools/audit_chest_pool.py` valida los pesos de las categorías de cofre,
  familias de armas y armaduras, además de dagas, puños, arcos, clubes, hachas
  y espadas.
- El VPK 054 pasa `check_vpk.py`: título `VAGV00006`, versión `00.54`, siete
  entradas, cero errores y cero advertencias.
- Dos corridas aisladas de Vita3K 0.2.1.4170 (SDL/Vulkan y Qt/OpenGL) agotaron
  sus límites de 90 y 120 segundos. Ninguna generó `vita3k.log` ni log nativo;
  por tanto no verifican el arranque. En la segunda apareció una ventana de
  error de Vita3K, pero Computer Use no pudo capturarla (timeout en ambas
  capturas), así que la causa exacta sigue sin comprobarse.

## Estado

La recuperación de los datos directos ya no es el bloqueo: falta expresar las
subtablas jerárquicas como un paquete de datos privado, seleccionar el tipo
según nivel y pesos, construir el objeto con sus atributos y propiedades,
mostrarlo en el mundo y conectar la recogida/equipamiento. No se incluye el
FLAIL en VWA mientras el renderer no soporte su secuencia y atlas `player-swing`.
La iteración no afirma que ACT_ONE sea un mundo completo ni jugable en Vita3K.
