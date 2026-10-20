# Iteración 131: auditoría del HP del Goblin King

Antes de conectar melee/arco al jefe revisé la ruta de salud en los exports de
Ghidra. `Entity::Entity` inicializa `hp` y `maxHp` en 1; el loader VWE3 también
usa `hp=1` para poder representar un registro del boss en pruebas. Ese valor no
es evidencia de salud nativa. `BossGoblinKing::hurt` delega a `Unit::hurt` y,
si la respuesta fue letal, crea recompensa y quita el jefe activo.

`BossGoblinKing::initializeStats` muestra operaciones con constantes 70.0,
1.5 y 8.0, y una rama NG+ con 16.0, 1.8 y 210.0. Sin embargo, el export de
Ghidra tipa los miembros heredados como `_padding_`, así que aún no se puede
atribuir esas constantes a `maxHp`, daño o defensa. Por fidelidad no habilité
daño contra el jefe mientras el registro use el sentinel 1; hacerlo volvería
cualquier golpe letal. La siguiente tarea de decomp es recuperar el offset y
flujo de datos de los campos Entity/Unit tocados por esta función y el efecto
de `Unit::hurt` antes de habilitar una pelea.

El comportamiento de carga y sus pruebas siguen en
[ITERATION130.md](ITERATION130.md); el smoke Vita3K anterior aún falla antes de
la sesión nativa. Esta auditoría no cambia el VPK ni afirma gameplay completo.
