# Iteración 068: RNG completo de libros de skills

El ledger de `floor_layout.py` ya modela por cada pedestal vacío la creación
del `Item` y el bucle de cinco selecciones de skill de
`ItemGenerator::generateNewRandomSpellBook` (`0x00643c90`). El PDB confirma que
`Skill+0x70c` es `spellTier`; `Skill::Skill` (`0x0078b150`) inicializa ese campo
en `-1` y lo cambia según el tipo. El umbral depende del piso. Cada selección
puede provocar otra llamada aleatoria dentro de `Item::setSkill` (`0x00623680`)
si el tipo nuevo no coincide con los cuatro bits bajos de `randIndex`. El
modelo conserva el orden por pedestal, incluidas las tiradas intercaladas de
constructor, skill y ajuste de índice.

Las skills seleccionables corresponden a los enum IDs 0–15; sus tiers se
transcribieron del constructor Ghidra y se verifican mediante el barrido de 20
semillas por cada piso ACT_ONE. El reporte de seed 12345/floor0 registra un
libro, nueve draws y los SkillType IDs `[13, 1, 15, 8]`, además del tier final
y el estado RNG posterior. Un caso controlado comprueba las guardas por tier y
la actualización de `randIndex`. `full_room_entity_rng_reconstructed` sigue en
`false`, porque aún
faltan otras tiradas de creación de entidades, el mapa bloqueante G/H y los
shuffles de endpoints.

La suite Python completa pasa 138/138 pruebas. No se modificó el runtime C y no
se compiló un VPK en esta iteración; el VPK 00.65 continúa siendo el último
verificado en Vita3K. Este avance mejora la reproducción determinista del
generador, pero el primer mundo todavía no está completo ni listo para una
prueba nueva del VPK.
