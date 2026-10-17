# Iteración 062: botín de cofres en el runtime

Se integra `VLT1` versión 2 al runtime ACT_ONE. El cargador valida descriptores,
rangos, pesos, referencias y el pool raíz; el selector recorre subpools
ponderados, filtra por nivel y excluye tipos únicos ya generados. Abrir un cofre
crea una entidad de drop reproducible por semilla local, el atlas y sprite se
dibujan desde los recursos privados del bundle, y ↑ recoge el objeto al
inventario. Los sprites con índice `-1` mantienen un marcador visible.

La salida privada `_local/world-data062/act-one` incluye el paquete de loot y
los siete atlas. Su manifiesto SHA-256 pasa validación con 64 archivos. Build
VitaSDK estricto produce
`_local/build-act-one-062/Vagante2Vita-iteration062.vpk`; `check_vpk.py` reporta
7 entradas, 0 errores y 0 advertencias. Suite Python actual: 136 pruebas
aprobadas. El bundle regenerado está en `_local/world-data062b/act-one` y el VPK
tiene 825.024 bytes, SHA-256
`EC40BCCEAA885631129DBEA972D6B490258E69E147C0848F90F72FF1886C34F0`.

Se añade prueba nativa C para leer VLT1, seleccionar un item único y rechazarlo
cuando el nivel no aplica; la prueba y la de interacción de jugador quedaron
integradas a CMake, pero esta máquina no tiene CMake/CTest disponible, por lo
que no se ejecutaron aquí.

Se cotejaron los sprites de las 170 filas con dimensiones de los siete atlas:
136 índices explícitos están dentro de la hoja y 34 defaults quedan marcados
con sprite ausente. `compile_item_loot.py --assets` ahora repite este control
contra el PNG privado en cada compilación; seis pruebas unitarias cubren bounds,
dimensiones faltantes y formatos.

El smoke aislado de Vita3K no crea un log de sesión después de 60 segundos.
stdout, stderr y `vita3k.log` están vacíos; el harness terminó el proceso con
código -1. Por tanto, el VPK compila y pasa auditoría estructural, pero todavía
no se declara iniciado ni probado en Vita3K. El runtime ahora requiere el
paquete VLT1 dentro del bundle ACT_ONE. El mismo resultado se reprodujo con
Vulkan y OpenGL en ejecuciones aisladas: ambas terminaron por timeout con
stdout, stderr y `vita3k.log` vacíos, lo que sitúa el fallo antes de que la
aplicación cree su propio log, sin atribuirlo aún al renderer ni al código del
juego. Se probó también el paquete INTRO (sin ACT_ONE) en el mismo clon y
produjo el mismo timeout sin logs. La instalación compartida contiene logs de
un run anterior con `EXCEPTION_ACCESS_VIOLATION`, pero no pertenecen a estas
ejecuciones aisladas y no se usan para atribuir el fallo actual.

## Referencias de port

`repentogxm` documenta recompilación estática de un ejecutable x86 a C/ARM,
manteniendo los recursos fuera del repositorio y exigiendo una copia legal del
juego. Su traducción de ejecutable no reemplaza la extracción actual de
Vagante: es una vía experimental distinta, con coste alto y atada al binario
exacto. La documentación de D2Vita describe un loader Win32/x86 y dynarec
Box86-derived para ejecutar el binario genuino de Diablo II; sus recetas VitaSDK,
pruebas host del runtime, diagnóstico de arranque, datos privados y límites
de distribución son reutilizables como prácticas, no como motor portable ni
extractor de Vagante. Este proyecto mantiene su runtime nativo y exports
verificables con Ghidra/PDB.
