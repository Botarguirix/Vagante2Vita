# Iteración 056: mapa de sprites para los pools de botín

`tools/extract_item_sprites.py` relaciona los tipos nombrados de las tablas
directas con el índice `spriteIndex` y la hoja que asigna
`Item::setNameAndDescription` (`0x00617290`). La salida local cubre los 170
tipos de los pools: 136 tienen índice explícito y 34 conservan el valor inicial
`-1`. También valida los nombres con el enum ItemType exportado del PDB. El
JSON queda en `_local/item-sprite-indexes.json` y no se copia al VPK.

## Verificación

- Extracción real desde la pseudodecompilación Ghidra y los enums locales:
  `verified=true`, 170/170 tipos requeridos, cero tipos faltantes.
- Pruebas Python: el parser conserva casos compartidos y valores por defecto;
  valida las hojas de armas, armadura, pociones, pergaminos y libros.
- La extracción de pools 055 sigue validada contra el ejecutable: 170 filas en
  18 tablas directas; `FLAIL` queda nombrado por el PDB.

## Pendiente

El índice ya permite preparar los iconos de los tipos que salen de los cofres,
pero falta comprobar cómo cada atlas traduce `frame=-1` y `spriteIndex` a un
rectángulo de 20 px, empaquetar las hojas privadas, y conectar la creación,
animación, recogida y equipamiento de `Item` al runtime. La generación de loot
no está integrada todavía y la prueba de arranque Vita3K de la 054 sigue sin
log nativo.
