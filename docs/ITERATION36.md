# Iteración 036: orientación de trampas ACT_ONE y loot investigado

Esta iteración conserva la investigación del generador de botín y corrige una
parte comprobable del runtime de ACT_ONE: VWE2 transporta la orientación
horizontal de `DartTrap`, pero el renderer la ignoraba y el cargador no aceptaba
el bit. Ahora el exportador conserva `facingLeft` junto a `canReload`, el
cargador valida ambos y el renderer refleja horizontalmente el sprite. El flujo
de fuego, proyectil, daño y recarga sigue pendiente.

## Evidencia recuperada

Ghidra terminó la decompilación enfocada de
`ItemGenerator::generateNewRandomChestItem` (`0x006434f0`) al abrir el proyecto
existente en solo lectura, saltar el análisis global y usar la clase Java
precompilada del script. Esto evita el error de acceso que impide a esta
instalación compilar scripts `.java` desde los JAR. El pseudocódigo adicional
está en `_local/item-generator-chest.c`; los binarios, PDB, proyecto Ghidra y
salidas siguen privados dentro de `_local`.

La función consume un sorteo del Mersenne Twister, elige `POOL_CHEST` (enum
PDB: 23), consulta `getRandomItemTypeFromPool` (`0x00645540`) y llama
`generateNewRandomItemOfType` (`0x006435d0`). `Chest::finishOpen`
(`0x00546210`) genera ese objeto solo cuando el cofre no tiene contenido y lo
añade al nivel. La llamada utiliza el nivel del cofre, un incremento
condicional de 9 y `magicFindModifier`.

El constructor `Chest::Chest` (`0x005457e0`) crea una caja física de 12×9 px y
la inicia cerrada y desbloqueada. La función `getRandomItemTypeFromPool` filtra
por rango de nivel, excluye tipos únicos ya obtenidos y puede descender a
subpools con sus propios pesos. El inventario real es una cuadrícula de 5×7:
`Inventory::pickUp` (`0x00609cc0`) comprueba `Item::isPickupable`, restricciones
de unicidad y espacio compatible antes de recoger. El PDB y el pseudocódigo
están exportados localmente; no se presentan como fuente recompilable.

## Qué permite y qué no permite concluir

Ya se conoce la secuencia de llamadas que une apertura, generación, creación,
aparición en el nivel y recogida. Aún no está recuperada con precisión toda la
tabla de probabilidades/niveles, el orden completo de llamadas RNG de cada
subpool, el mapeo de todos los IDs a sprites/atributos, ni las reglas de stack,
equipo y capacidad del inventario. Sin esos datos, recompensar el cofre con un
objeto fijo o elegir un ID aleatorio no sería fiel al original.

El runtime ACT_ONE sigue mostrando el cofre abierto, pero no crea ni recoge
objetos. El constructor de `DartTrap` inicializa `facingLeft=false`,
`hasFired=true` y `canReload=false`; `setEntityState` recibe `facingLeft` en el
bit 0, `hasFired` en el bit 1 y `canReload` en el bit 2. El exportador conserva
`facingLeft` y `canReload`. Las definiciones de plantillas no indican
`facingLeft`, así que se usa el default `false`; queda pendiente confirmar si
otra fase de creación del nivel ajusta ese estado. El bit `hasFired` no está
serializado.

No se activa fuego/daño de la trampa: el pseudocódigo de Ghidra usa un
`Projectile` físico 3 y somete la colisión a estado de actor, entidades y
segmentos del cuerpo. Un daño instantáneo simplificaría indebidamente esas
reglas. Tampoco se genera botín ni se declara completo ningún piso. La próxima
etapa debe recuperar la orientación y los estados iniciales de la entidad desde
la definición del nivel, y completar el proyectil/daño y la ruta de items con
fixtures observables.

## Verificación

`python -m unittest discover -s tests`: 118/118. `git diff --check` no reportó
errores de whitespace. El VPK se compiló con VitaSDK Windows desde la fuente
00.36, tiene siete entradas permitidas, `VAGV00006`, versión `00.36`, 807 609
bytes y SHA-256
`4afa5e02a15222e9cc86669d097d0194a747eed4aa10379ac4f281fa813b0a58`.
El contenedor y SFO se revisaron con ZIP; no se pudo ejecutar el validador
oficial del repositorio porque el script `tools/check_vpk.py` no está presente
en este checkout.

El smoke aislado se lanzó con bundle ACT_ONE 049 en
`_local/vita3k-iteration036`. El harness instaló los siete archivos del VPK en
el filesystem privado, pero Vita3K agotó 60 segundos y se detuvo el proceso
clonado con código -1 antes de crear `vita3k.log` o un log de sesión nativo.
stdout/stderr están vacíos. No cuenta como arranque ni gameplay validado; el
fallo sigue en la inicialización del emulador en este equipo. WSL
continúa respondiendo `E_ACCESSDENIED`, así que tampoco se pudo compilar y
ejecutar `world-entities-test` de forma nativa en este ciclo. La nueva prueba
C de flags está escrita pero queda pendiente de ejecutar cuando haya toolchain
host disponible.
