# Iteración 01.53: recuperar la selección de acciones del Dragon

Se contrastaron los pseudocódigos locales de Ghidra para
`BossDragon::step` (`0x0050c1c0`), `addNewActions` (`0x00510040`) y
`addAction` (`0x00510200`) con el enum PDB extraído en
`_local/analysis010/enums.tsv`. La documentación del ledger ahora registra
los 15 valores `DragonActionType` y las dos formas principales de llenar la
cola: secuencia de vuelo/buceo con condiciones y tiradas `%3`, o la rama común
de disparo opcional seguida por `MOVE_PLAYER(20)` y `FACE_PLAYER`. El dispatcher
de `step` conecta los IDs de disparo con `shootFireballsAtPlayer(3/1)` y los de
buceo/carga con sus métodos dedicados.

También se cotejó el `switch` de `step`: IDLE (0) consume su duración; las
acciones 1–3 fijan orientación; MOVE_PLAYER (4) calcula un vector al objetivo y
termina al entrar en su umbral de distancia; PATH_PLAYER (5) espera visibilidad
o solicita `pathToUnit`; los disparos son 6/7; AIR_FLIP (10) reinicia el sprite
de buceo antes del ataque; DIVE_BOMB (11), CHARGE_PLAYER_GROUND (13) y
CHARGE_REFLECT_GROUND (14) llaman rutinas distintas. FLY_UP (8), LAUNCH_UP (9)
y FLY_HIGH (12) tienen temporizadores/condiciones adicionales que requieren
resolver los campos de estado antes de portarlos.

En `shootFireballsAtPlayer` también quedó confirmado el gate auxiliar: consulta
visibilidad a 20 unidades y `Entity::testLine`; esta última traza los tiles de
`drawLine(..., false)` y falla si `Level::isSolid` encuentra un sólido. El
disparo espera el sprite `spit` y consume `generate_canonical<float>` del
MT19937 para alterar la coordenada vertical de apuntado. Los vectores de
proyectil reciben un factor 1.45 o 1.8125 según un byte de estado aún sin
identificar. Los argumentos 3 y 1 de las acciones no se consideran cantidad de
proyectiles: su papel de selector de estado/animación requiere confirmación.

Esto identifica la divergencia que debe corregirse: el runtime actual reemplaza
esa cola por persecución homing y un temporizador fijo de bolas de fuego. No se
copió todavía la lógica al runtime porque falta confirmar el significado de
varios campos superpuestos del pseudocódigo y el estado exacto del MT19937. La
rutina de disparo usa además el sprite y la visibilidad del juego original, que
el runtime actual no modela para el Dragon.

No cambió el ejecutable Vita ni se generó un VPK. La última prueba aislada de
Vita3K para el VPK 01.50 terminó antes de la sesión nativa con `0xC0000005`
(véase [ITERATION152.md](ITERATION152.md)). ACT_ONE sigue incompleto y sin una
replay nativa validada.
