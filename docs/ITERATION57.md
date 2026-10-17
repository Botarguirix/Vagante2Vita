# Iteración 057: build reproducible y auditoría del botín de cofres

Se compiló el código actual como VPK `00.57` para que las pruebas no dependan
del artefacto antiguo 054. `check_vpk.py` acepta el paquete: TITLE_ID
`VAGV00006`, siete entradas, cero errores y cero advertencias. La suite Python
actual pasa 125 pruebas.

## Evidencia de cofre en Ghidra

`Chest::finishOpen` (`0x00546210`) llama a
`ItemGenerator::generateNewRandomChestItem` cuando el cofre no tiene contenido
preexistente. `generateNewRandomChestItem` (`0x006434f0`) consulta
`POOL_CHEST` (enum 23), invoca `getRandomItemTypeFromPool` y crea un Item con
el nivel y modificador de magia recibidos. El selector filtra por nivel,
rareza y tipos únicos; por ello las 170 filas de los 18 pools directos no
equivalen a la tabla de cofres. El runtime actual sólo marca el cofre abierto
y cuenta la interacción: el botín aún no se materializa ni se recoge.

## Vita3K

La VPK 057 se lanzó desde la CLI en el perfil aislado
`_local/vita3k-act-one-057`, con el paquete privado ACT_ONE de 56 archivos
validado antes del arranque. El proceso terminó con código `-1` al agotarse
60 segundos; no se creó sesión nativa. `stdout.log`, `stderr.log` y
`vita3k.log` no contienen un diagnóstico de aplicación. Esta corrida no
confirma que el juego arranque ni atribuye el bloqueo a la VPK o al paquete.

La prueba de salida automatizada necesita una sesión nativa para verificar
replay; por tanto, sus aserciones de gameplay no se ejecutaron en esta corrida.
El bloqueo de arranque permanece pendiente. La VPK no incluye recursos
comerciales; el paquete del usuario se entrega por separado en el perfil de
prueba.
