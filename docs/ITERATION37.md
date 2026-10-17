# Iteración 037: orientación ACT_ONE derivada de la geometría

Ghidra confirma que `DartTrap` determina su orientación al asignarle un nivel:
su constructor (`0x00593040`) convierte el centro del cuerpo 16×16 a tile con
`floor((posición + 8) / 16)`. `setLevel` (`0x00593250`) consulta si el tile a
la derecha es sólido; si lo es, el disparo apunta a la izquierda. El helper
`expandToViewRangeAndResetHasFired` (`0x00594980`) busca el límite del corredor
hasta seis tiles. El JSON de sala no contiene una orientación autoritativa.

El exportador ACT_ONE ahora deriva el bit `facingLeft` de la geometría VWI2,
usando los mismos tipos sólidos del runtime (tile 1 y 7). Conserva `canReload`
en el bit 0 y serializa la orientación calculada en el bit 1 de VWE2. La prueba
de regresión cubre una trampa con pared a la derecha y una con corredor abierto;
también cubre el offset de centro de 8 px del constructor. En el piso base
actual, la única trampa está en `(496, 432)`, su tile derecho es abierto y apunta
a la derecha, así que el paquete coincide con el bit que ya tenía.

## Recuperación Ghidra para el siguiente cambio

`DartTrap::step` (`0x00593530`) revela niebla mientras está armada. Para
trampas recargables restaura el tile de la trampa y espera 130 pasos antes de
avanzar la animación de recarga; al completarla reinicia `hasFired`. Las trampas
sin recarga mueren si el tile que las sostiene deja de ser sólido.
`DartTrap::fireDart` (`0x00594560`), contrastado con su listing x86 y los enums
exportados del PDB, crea un `PROJECTILE_ARROW_TRAP` (enum 3), con velocidad
horizontal ±5 px/paso, velocidad vertical 0, daño físico 5 y facción neutral.
`Projectile::step` (`0x00721fe0`) incluye ArrowTrap en el movimiento genérico de
flecha y ejecuta la lógica de combate. La pseudodecompilación de `fireDart`
mezcla algunos temporales optimizados, por eso estos argumentos se contrastaron
con los pushes de la llamada y los valores del PDB; no se trata como código C++
recompilable.

La orientación exportada es el único comportamiento de trampa integrado en esta
iteración. El runtime aún no activa el proyectil, la colisión, el daño ni la
recarga; el siguiente cambio debe modelar ese ciclo y sus contactos con la
física de jugador antes de incluirlo como comportamiento jugable.

## Verificación y estado

- `python -m unittest discover -s tests`: 119/119.
- `tools/export_act_one_world.py` regeneró el piso base 80×40; VWI2 conserva
  hash `e0f5104f33aac78766b1da445c6c18fd8b33b4e4e15e7e5f78b1de0b5486be61` y
  VWE2 hash `8a8dd8fbff652ddebaa4a1f6b49f520b83973f793e9e102aeac4bdd8c103f68b`.
- El bundle externo privado `_local/delivery-037-smoke/world` valida 56 archivos.
  Recursos propietarios siguen fuera del VPK.
- VPK compilado con VitaSDK Windows: `VAGV00006`, versión `00.37`, siete
  entradas, 807 612 bytes, SHA-256
  `547a227ce080cb29f7fbaf3ac1239fd077f71690bb34ec7a3ca099583a78b5e0`.
- Smoke en la copia aislada `_local/vita3k-iteration037`: instaló el VPK y
  preparó la replay de salida, pero Vita3K agotó 60 s y terminó con código -1
  antes de crear `vita3k.log` o sesión nativa. No valida arranque ni gameplay.

El informe exportado conserva `exact_generation_parity=false` y
`floor_complete=false`; enemigos, loot, trampas activas, recorrido físico de los
tres pisos y transición final siguen pendientes.
