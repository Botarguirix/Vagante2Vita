# Iteración 029: primer pase de enemigos ACT_ONE

Se recuperaron del binario/PDB local las listas de tipos por acto y los pesos
de spawn. ACT_ONE admite Goblin (9), Bat (10) y Lurker (11); Goblin, Bat,
Lurker y Slime tienen peso 1.0. Las ramas del generador original y sus
comprobaciones geométricas ya están parcialmente documentadas en
`GENERATION_RECOVERY.md`, pero `Room::monstersAllowed` continúa siendo un stub.

El generador crea como máximo un enemigo provisional en cada sala ACT_ONE
simple que pasa verificaciones de suelo, ruta a borde y distancia de entrada y
salida. Usa las probabilidades originales Goblin 40%, Bat 45%, Lurker 15% para
la rama de suelo sólido bajo cavidad. El número de enemigos y el consumo de RNG
no coinciden aún con Vagante original.

El paquete privado VWE3 separa instancias de enemigos de los objetos VWE2.
El runtime añade movimiento horizontal de persecución, gravedad y colisión de
tile, dos puntos de vida provisionales, cooldown contra golpes repetidos y daño
con la geometría/datos de las armas actuales. Renderiza siluetas de color.
No hay ataques enemigos ni daño al jugador, animaciones originales, loot,
trampas activas ni transición al siguiente piso. No se debe presentar esta
iteración como el primer mundo completo; `floor_complete=false` se conserva.

La recuperación de cofres confirma que `Chest::finishOpen` (`0x00546210`)
solicita un tipo de `POOL_CHEST` (`23`) a
`ItemGenerator::generateNewRandomChestItem` (`0x006434f0`), crea el objeto y lo
añade al mundo; `Inventory::pickUp` está en `0x00609cc0`. El constructor de
`ItemGenerator` ya se pudo decompilar en pseudocódigo Ghidra privado. La entrada
23 agrupa los pools de armas, armadura, otros objetos y varitas. Faltan
resolver y contrastar ponderaciones, niveles y tipos concretos, además de
implementar entidades `Item`, inventario y su representación en el runtime.
Por eso el VPK aún no concede recompensas; no se sustituye el pool por un
objeto inventado. El pseudocódigo es evidencia para reconstrucción, no código
fuente recompilable.

Para avanzar al mundo completo todavía hacen falta: ejecutar el probe de ruta
física en host contra `player_step`, reconstruir las reglas de salas y entidades
con la semilla original, implementar daño/muerte y las trampas según Ghidra,
recuperar pools e inventario para que los cofres entreguen objetos reales, y
probar salida/transición entre pisos. El probe de ruta ACT_ONE compila ahora
como ELF ARM con VitaSDK (74 624 bytes de texto); eso solo prueba compilación.
No se puede ejecutar en este host: `arm-vita-eabi-run` también falla con un ELF
vacío y WSL devuelve `E_ACCESSDENIED`.

Validación: 107/107 pruebas Python pasan. El runtime y los tests nuevos de
combate/VWE3 compilan con VitaSDK usando `-Wall -Wextra -Werror`. El VPK 00.29
se construyó y el paquete externo tiene 56 archivos, incluyendo layout e
informe `floor_complete=false`, y pasa la verificación de hashes.
El VPK pesa 805076 bytes; SHA-256
`F21A1C1CACBA12DB18D4C72975EE27A3BB45FBF68700218ED59CF6BA94A14485`.

El harness crea replay de salida de 582 ticks e instala el VPK en la instancia
aislada `_local/vita3k-vagante-clone`. Vita3K 0.2.1.4183 termina con
`0xC0000005` antes de crear ventana o log nativo; por tanto, este replay aún no
verifica carga de mundo, enemigos, combate ni salida. No se tocó `G:\Vita3k`.
`arm-vita-eabi-run` devuelve 108 tanto para el test como para un ELF vacío,
por lo que ese runner no sirve para validar esta lógica en el entorno actual.
No hay CMake/compilador host disponible; no se declara ejecución de combate en
emulador, host ni hardware.
