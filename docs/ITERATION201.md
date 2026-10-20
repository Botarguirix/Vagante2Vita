# Iteración 201 — evidencia Ghidra del golpe al jugador

## Hallazgo

Se revisaron las exportaciones locales de Ghidra para `Player::hurt`
(`_local/decompiled/006f9020.c`, `0x006f9020`), `Player::doCollision`
(`_local/decompiled/006f74c0.c`, `0x006f74c0`) y `Player::applyImpulse`
(`_local/decompiled/006fc9f0.c`, `0x006fc9f0`). Tras `Unit::hurt` con resultado
`DID_HURT`, `Player::hurt` pone `hurtTimer` a 10, libera `hanging` y aplica una
reacción condicionada por `immuneToKnockback`, `isSkeleton`, `rangeFocus` y el
tipo de fondo. `Player::doCollision` consulta una tabla `invulFrames` por
entidad al procesar colisiones.

La tabla por entidad no demuestra invulnerabilidad global del jugador y no
evitaría por sí sola impactos de fuentes distintas, como el Worm y el Bat en la
traza. Falta recuperar `Unit::hurt` y mapear los argumentos de knockback antes
de implementar la reacción. No se cambió el daño ni se añadió inmunidad
inventada.

## Estado de la cadena

El probe con melee cercano volvió a pasar los proofs físicos independientes de
los tres pisos en `_local/route-act-one-201-baseline-recheck/`. El piso 1 vence
al Worm y sale en 1,558 ticks con 4 HP cuando empieza con 80 HP. La cadena
persistente correspondiente está en `_local/host-chain-201-baseline/`: entra
al piso 1 con 10 HP y muere al tick 850; recibe 4 y 3 de daño del Worm y 3 del
Bat, mientras el Worm conserva 94.8 HP. El proof independiente de ese piso
comienza con carga completa, lo que el verificador reporta como estado
incompatible. Una prueba aislada que empieza con 20 HP muere en el tick 934.
Las variantes de evasión automática del Dragon probaron la traza sin mejorar
una cadena completa y se retiraron. La prueba previa de Vita3K sigue
deteniéndose antes de cargar el VPK, así que esta iteración no reconstruye el
paquete ni reclama validación del emulador.

La evidencia y sus límites también están resumidos en
`docs/GENERATION_RECOVERY.md` y `docs/ITERATION200.md`.
