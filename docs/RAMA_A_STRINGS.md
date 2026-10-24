# Tablas de nombres — avance opcional de tarea 3

2026-10-24, rama `rama-a`. Sin push. Cabeceras generadas sin cambios.

| Función del alcance | Estado |
|---|---|
| `Entity::initEntityStringIdentifiers` @005aab80 | Traducida en `Strings.cpp`: 124 asignaciones en el orden original, 123 entradas únicas y tabla inversa. `TYPE_SPELL = "Spell"` aparece dos veces en el original y se conserva. |
| `ItemAttribute::initAttributeStrings` | Traducida: 101 nombres y sus inversas, desde `ATTRIBUTE_PRIMARY_BEGIN` hasta `NUM_ATTRIBUTE_TYPES`, ambos incluidos. |
| Lambda interna de ItemAttribute @00635e60 | Integrada como lambda C++: aviso de duplicado, asignación directa e inversa; se conserva la comprobación final de cobertura. |
| `Skill::initSkillStringIdentifiers` | Traducida: 27 nombres traducibles y tabla inversa con los textos traducidos. |
| `Stats` | Pendiente. Esta entrega opcional cubre únicamente las tres tablas; no se han escrito cuerpos parciales de Stats ni se declara terminada toda la tarea 3. |

## Evidencia y detalles conservados

Las asignaciones de entidades y atributos se contrastaron con el pseudocódigo
en la carpeta externa del usuario. Los textos de los últimos diez skills no
estaban disponibles en los argumentos recuperados por Ghidra: se consultó
`tools/call_args.py Skill::initSkillStringIdentifiers --callee boost::locale::translate<char> --before 8`
y se leyeron sus direcciones EDX con `PE.cstring` del ejecutable original.

No se deducen los nombres desde el enum: `ICE_BOLT` es `Iceball`, `TELEPORT` es
`Blink`, `HEAL` es `Circle of Light` y `DOG_WHISTLE` es `Whistle`, por ejemplo.
Se usa el facet de catálogos externos `recon::translate`, ya empleado por Config
e Input, en lugar de depender de Boost.Locale/MSVC.

Los mapas no se vacían antes de reinicializar: los mapas inversos retienen
entradas anteriores, incluidas traducciones antiguas, como en el original.
Las tablas enum→string se recorren por orden de enum al construir las inversas.
No hay llamadas al RNG ni se modifican constantes de otras unidades.

## Verificación

- `pwsh tools/build_recon_pc.ps1 -Run`: compiló/enlazó, abrió la ventana nativa y
  terminó al cerrarla normalmente con **exit code: 0**. El `recon.log` alcanzó
  52 funciones pendientes; ya no se alcanzan stubs de los tres inicializadores.
  Log: `_local/sprite-vfont/strings-build-run.log`.
- `pwsh tools/run_recon_tests.ps1 -Unit Strings`: pasó tamaños, tablas inversas,
  todos los atributos 0–100, sentinels, nombres recuperados del ejecutable,
  traducción mediante el facet actual, reinicialización, conservación de las
  entradas antiguas y estado del MT19937 sin cambios.
  Log: `_local/sprite-vfont/strings-test.log`.
- La comprobación del índice tras las entregas 5 y 6 no encuentra ninguna
  función con pseudocódigo pendiente de Sprite o VFont. Sus helpers sin cuerpo
  independiente y adaptaciones se detallan en los informes de esas unidades.
- No hay validación nueva de gameplay ni de hardware Vita, ni datos comerciales
  añadidos al repositorio.
