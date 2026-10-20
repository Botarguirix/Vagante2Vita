# Iteración 122: ledger de evidencia para los jefes ACT_ONE

Se aplicó al análisis local la distinción de Decompedia entre matching
decompilation y reconstrucción funcional para un port. VaganteVita no cuenta
con los insumos del compilador original ni compara el binario recompilado, así
que el avance de Ghidra se documenta por función y evidencia, no como un
porcentaje de decompilación equivalente.

La revisión del estado actual confirmó que `choose_boss_room` reconstruye la
sala seleccionada, `boss_room_spawn_geometry` reporta anclas estáticas, y la
traza de `addRandomEnemies` marca dónde ocurriría la llamada. Sin embargo,
ninguna de esas piezas ejecuta ni serializa al jefe: el estado RNG exacto en
`addBoss`, los filtros de entidades dinámicas, los tres ciclos de combate y la
continuación tras derrota siguen pendientes. El nuevo
[ledger de jefes](BOSS_DECOMP_LEDGER.md) asigna para cada función dirección,
hallazgo confirmado, incertidumbre y evidencia local; también define los
gates para que el próximo cambio pueda conectar datos con runtime sin elevar
una aproximación a resultado nativo.

El paquete privado ahora copia los 23 atlas de los tres jefes ACT_ONE a
`bosses/{goblin_king,dragon,worm}/`, separados de los cinco enemigos normales.
El export de `BossGoblinKing::setLevel` también permite recuperar sus celdas,
argumentos de velocidad y offsets de los 13 roles de Sprite. Se registraron en
`tools/boss_goblin_king_sprites.json` sin afirmar que `speed_arg` equivalga
directamente a cuadros por segundo. `Sprite::step` (`0x007b9720`) confirma que
el temporizador avanza una vez por tick y el cambio ocurre cuando supera
`speed`, por lo que el intervalo es `speed + 1` ticks; el loop de cada acción
todavía debe verificarse. El auditor calcula cuadros a partir de las dimensiones reales
del atlas y rechaza hojas no divisibles por sus celdas. Los PNG todavía no se
cargan desde el runtime ni implican que los jefes aparezcan. La prueba del
empaquetador verifica assets de cada familia. El auditor de recursos ahora
valida los 23 PNG si encuentra el namespace de jefes, y el bundle privado
`_local/delivery-act-one-campaign122` se creó desde el bundle 112 conservando
los tres pisos, rutas y manifiesto; se añadieron las hojas originales y sus
hashes SHA-256. El preflight del manifiesto confirma 137 archivos; el auditor
comprueba 89 imágenes (23 atlas de jefe, 13 roles de Sprite para Goblin King)
y deriva por hoja 1–4 cuadros desde la celda nativa registrada. Los intervalos
calculados son 11 ticks para speed 10, 6 para speed 5 y 7 para speed 6.

## Validación y alcance

- Revisado el árbol de trabajo, `docs/ROADMAP.md`, `docs/GENERATION_RECOVERY.md`,
  el export Ghidra `0x0068a770`, `tools/floor_layout.py`,
  `tools/floor_room_selection.py`, `tools/export_act_one_world.py`, y el
  cargador/runtime VWE3.
- No se modificó la lógica de juego ni se produjo VPK: el análisis mostró que todavía
  no hay contrato de spawn seed-exacto ni runtime de jefe, por lo que añadir
  una instancia aproximada al paquete presentaría como ejecutable una
  reconstrucción que el generador no puede respaldar.
- Suite Python completa: 187/187.
- La suite de empaquetado pasa 11/11; el auditor integral del bundle ACT_ONE
  verifica 23 atlas de jefes junto con el runtime existente.
- La validación de jefes en Vita3K sigue pendiente; los controles anteriores
  no iniciaron una sesión nativa.
