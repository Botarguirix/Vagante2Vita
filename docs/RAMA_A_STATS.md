# Stats — cierre de la tarea 3 con pseudocódigo

2026-10-24, rama `rama-a`. Sin push y sin modificar cabeceras generadas.

`recon/src/Stats.cpp` traduce los 16 nombres de método del índice (17 cuerpos,
porque `getKilledCount` tiene dos sobrecargas), las tres lambdas de registro y
las 13 condiciones de juego almacenadas en std::function. Se recuperan también
las dos constantes necesarias que estaban como TODO en statics.cpp.

## Funciones del índice

| Función | Estado |
|---|---|
| `Stats::getProgressionInterval` @007ba390 | Traducida; intervalo de puntos, límite superior inclusivo y saturación al último intervalo. |
| `Stats::init` @007ba420 | Traducida; 39 registros: 26 por progresión, 13 por condición; carga y actualización posterior. Construcción/check de logros Steam excluidos con comentario. |
| `Stats::load` @007bc600 | Traducida; JSON versión 1 y claves antiguas, en el orden recuperado. |
| `Stats::save` @007bcd80 | Traducida; JSON con PrettyWriter, orden original de secciones y mapas ordenados. |
| `Stats::isUnlocked` @007bd280 | Traducida; consulta sin insertar una clave ausente. |
| `Stats::setValue` @007bd2c0 | Traducida; hook Steam vacío con comentario. |
| `Stats::getValue` @007bd320 | Traducida; cero para clave ausente sin inserción. |
| `Stats::setIfMax` @007bd3d0 | Traducida; una clave nueva admite también valores negativos. Hook Steam excluido. |
| `Stats::increment` @007bd490 | Traducida; admite incrementos negativos. Hook Steam excluido. |
| `Stats::runIncrement` @007bd550 | Traducida; ignora slots fuera del vector y no redimensiona. Hook Steam excluido. |
| `Stats::getRunStat` @007bd640 | Traducida; devuelve cero si no hay slot/clave. Se protege también el slot negativo. |
| `Stats::accumulateKills` @007bd6d0 | Traducida; solo incrementos positivos, mapas normal/hard separados. Hook Steam excluido. |
| `Stats::getKilledCount(string,bool)` @007bd7a0 | Traducida; consulta el mapa seleccionado sin insertar. |
| `Stats::getKilledCount(int,bool)` @007bd830 | Traducida según el cuerpo compilado; resuelve EntityType y consulta bajas normales. Ver anomalía de especialización debajo. |
| `Stats::unlock` @007bd8e0 | Traducida; claves desconocidas producen log; eventos únicos para desbloqueos nuevos. |
| `Stats::updateUnlocks` @007bda70 | Traducida; limpia eventos, aplica puntos, recorre condiciones por orden de clave y evalúa una copia del Unlockable. |
| `Stats::getAvailableBackgrounds` @007bdcb0 | Traducida; filtros por clase, override, exclusión de Synchronized y orden unlocked primero/enum después. |
| `<lambda_3913835ec4361b51f3f24690c67fd942>::operator()` @007bc0e0 | Integrada como lambda C++ `add`: tipo, nombre y unlocked=false. |
| `<lambda_10d88fdacd14899dd479abe35c25d32c>::operator()` @007bc250 | Integrada como `progression`: puntos acumulados, nivel y descripción traducida. |
| `<lambda_9a2efa6f1662ccda8cf57690f12047a6>::operator()` @007bc520 | Integrada como `conditional`: descripción, std::function, hasCondition y nivel cero. |

## Condiciones recuperadas

Los _Do_call de MSVC contienen lógica de juego; esa lógica sí se traduce como
lambdas C++, sin copiar las vtables/refcounts del wrapper STL.

| Clave | Condición y evidencia |
|---|---|
| `boss_trophy` | `good_victory_count > 0`; lambda `08d4a6ed276c54065dd0787b38403e4a`. |
| `tinkerer` | Bajas normales de TYPE_BOSS_SPRIGGAN > 0; lambda `7b4da039f171d2e59feab8bb921de3e1`, ECX=0x35. |
| `death` | `mortality_floors > 2`; lambda `379e48d69b169ff530ee77d844500983`. |
| `knight_armored` | `Knight_victory_count > 0`; lambda `7c1201fb17a38f604468f3fbd44df193`. |
| `rogue_boomerang` | `Rogue_victory_count > 0`; lambda `5e72581d6bf2bda376e1e789b5facb00`. |
| `mage_heirloom` | `Mage_victory_count > 0`; lambda `4f1a1cd9639706bfd8ca81b1a608718e`. |
| `wildling_weapon` | `Wildling_victory_count > 0`; lambda `691da4fd23678267378647ab25472d2a`. |
| `houndmaster_flail` | `Ranger_victory_count > 0`; lambda `1ef9a453d80f87947c529f2a04419b69`. Se conserva el nombre de clave antiguo. |
| `music_cave` | Bajas normales de jefes 49, 50, 51, en ese orden; lambda `fad9b683047071079b5e5430c6d4fa3e`. |
| `music_forest` | Jefes 53, 52, 55; lambda `f4a7e17f97e8aee998c79dc5658bb98e`. |
| `music_catacombs` | Jefes 58, 59, 57; lambda `ffc8dfb056f8aaa53c728882d1254cd5`. |
| `music_rift` | Jefes 61, 62, 63; lambda `00364de33ba4567223b313d9faee319c`. |
| `hard_mode` | `victory_count > 0`; lambda `f95a99263e082e8bdd8ec2617e9e45de`. |

Las cadenas faltantes en Ghidra se recuperaron de inmediatos EDX/constantes de
`Stats::init` con tools/call_args.py, desensamblado y PE.cstring. Las tres
comparaciones de cada tema conservan el orden y cortocircuito originales.

## Constantes y comportamiento conservado

- `FILE_FORMAT_VERSION = 1`, comprobación de load y literal de save.
- `FILE_NAME = "savedata.sav"`, inicializador dinámico @00405cd0.
- Cada nivel añade `int(powf(float(level), 0.8f) * 10.0f + 100.0f)` antes de
  incrementar level. Los 26 umbrales comprobados son:
  `100,210,327,451,581,717,858,1005,1157,1314,1477,1645,1818,1995,2177,2364,2555,2751,2951,3156,3365,3579,3797,4019,4246,4477`.
- Se conserva la diferencia en el borde: getProgressionInterval(100) devuelve
  (0,100), pero updateUnlocks desbloquea el primer nivel con puntos >=100.
- init limpia unlocks pero no unlockPointThresholds, acumuladores ni nivel/puntos
  globales. Una segunda inicialización añade otros 26 thresholds, igual que el
  original. No se corrige silenciosamente esta peculiaridad.
- updateUnlocks no reduce progressionLevel y no revoca desbloqueos al bajar
  puntos. Copia std::function antes de evaluarla, igual que el ejecutable.
- El parámetro bool de `getAvailableBackgrounds` incluye todos los backgrounds
  como disponibles y salta los filtros de clase; clase 6 sigue devolviendo vacío.

## Anomalías de descompilación y firmas

1. Las etiquetas BackgroundType importadas por Ghidra están desfasadas en varias
   entradas. Se contrastaron las escrituras `movb ...,0x64(%eax)` y se usaron los
   nombres de las cabeceras PDB: Potion Addict=0x14, Stoic=0x15, Boomerang=0x17,
   Tinkerer/Grappling Hook=0x16 y Flail Wielder=0x13. El filtro por clase 4 compara
   0x13, por lo que filtra BG_FLAIL y no BG_POTION_ADDICT. No se editan enums.
2. La firma PDB de `getKilledCount(int,bool)` conserva un bool, pero la versión
   enlazada observada solo recibe el tipo en ECX y fuerza false antes de llamar
   a la sobrecarga string: `xor bl,bl` @007bd89d y `mov cl,bl` @007bd8a5. Se
   conserva esa especialización y se documenta que no prueba la semántica de
   una hipotética variante genérica hard=true no presente en el índice. La
   sobrecarga string sí distingue ambos modos. Cabecera sin cambios.
3. El literal `Defeat ???` de Trophy procede del ejecutable; no es texto
   provisional inventado para la reconstrucción.

## Persistencia y adaptación portable

Se usan RapidJSON 1.1 y streams STL. Orden de escritura: file_format_version,
points, accumulators, deaths_to_count, kill_count y unlocks. Solo se escriben
unlocks verdaderos. El original no guarda killCountHard, runStats ni
progressionLevel: tampoco se añaden al formato aquí.

`integers` tiene prioridad sobre `accumulators` si ambos son objetos. Claves
`die<Entidad>` migran a diedToCount solo para entidades conocidas; `unlock_...`
positivo habilita un registro conocido. Las secciones modernas se aplican
después y pueden sobrescribir esos valores. No se vacían los mapas al cargar.
Una versión desconocida bien parseada devuelve true sin cargar los campos, como
el cuerpo recuperado.

Se rechazan raíz JSON no objeto y errores de sintaxis; se omiten contadores de
tipo incorrecto, en lugar de reinterpretar sus bits como int. Se protege
getRunStat para índices negativos y se devuelve false ante fallo de cierre del
stream de salida. Son adaptaciones de entradas inválidas/E/S, no cambios a las
reglas para datos válidos. El formateo traducido soporta el token simple {1}; no
se reconstruyen todas las opciones de formato de Boost.Locale.

Steam Achievements está excluido mediante hook vacío con comentario. La revisión
de Achievements::check @004067a0 confirmó que ese hook no ejecuta updateUnlocks.
No se añaden APIs Windows, Steam ni llamadas al RNG.

## Verificación ejecutada

- `pwsh tools/build_recon_pc.ps1 -Run`: regeneró stubs, compiló/enlazó y ejecutó
  la ventana PC; cierre normal, **exit code: 0**. Log:
  `_local/stats/build-run-2.log`. `recon.log`: **51** funciones pendientes,
  ninguna llamada de Stats.
- Había un ejecutable previo abierto sin ventana que bloqueaba el enlace. Se
  conservó su imagen en `_local/build-recon-pc/vagante-in-use-7004.exe`, sin
  terminar aquel proceso, y se enlazó/ejecutó el binario nuevo para la prueba.
- `pwsh tools/run_recon_tests.ps1 -Unit Stats`: pasó. Verifica los 39 registros,
  todos los umbrales y predicados, límites, filtros/orden de backgrounds,
  counters y separación normal/hard, copia de callbacks, eventos únicos,
  round-trip JSON, migración legacy/prioridades, claves escapadas, entradas
  inválidas, versiones desconocidas, fallo de E/S, traducción y MT19937 intacto.
  Evidencia: `_local/stats/test.log`; fixtures nuevos bajo
  `_local/recon-tests/Stats/work/run-*`, sin modificar el guardado original.
- Comprobación del índice: no quedan métodos Stats:: con pseudocódigo sin
  definición; las tres lambdas nombradas se integran en init.
- No se genera VPK ni se declara validación de gameplay/Vita.

## Declaraciones sin cuerpo independiente

`runResetStats`, `getRunStatMaxPlayerSlot`, `debugPrintRunStats`, `diedTo` y
`getDiedToCount` no aparecen en la lista del índice de la unidad con pseudocódigo.
Se conservan como stubs hasta recuperar sus expansiones en llamadores. No se
inventan el tamaño de runStats ni las reglas de sus resets; las pruebas fijan
explícitamente el vector desde el fixture. Esto limita el alcance a los cuerpos
recuperados que exige la regla común y no equivale a declarar terminado todo el
sistema de progresión del juego.

## Revisión después de Background y GUI (2026-10-24)

La solicitud posterior vuelve a incluir Stats tras Background, GuiElement,
GuiButton, GuiTextBox y GuiWindow. Se ejecutó de nuevo --unit Stats y se
contrastaron los 16 nombres/17 cuerpos y las tres lambdas con Stats.cpp: no
queda ninguna función con pseudocódigo de la unidad sin traducir. La traducción
original ya estaba en el commit 78f5ade; no se duplica ni se altera su lógica.

Se añade una prueba de integración: todos los Unlockable::BACKGROUND del
registro crean un Background válido con nombre/descripción; se comprueban los
valores 19–23 contra los nombres recuperados (Flail Wielder, Potion Addict,
Stoic, Tinkerer y Boomerang), sin alterar RNG. Se conservan los cinco helpers
sin pseudocódigo independiente y las anomalías documentadas arriba.

Verificación final con todas las unidades presentes:
`pwsh tools/build_recon_pc.ps1 -Run` compiló, ejecutó y cerró normalmente con
**exit code: 0**. recon.log registra 47 funciones aún sin traducir del resto del
programa; ninguna llamada de estas seis unidades durante ese arranque.
`pwsh tools/run_recon_tests.ps1 -Unit Stats` pasó, incluida la integración
Background. Logs locales: `_local/gui/Stats_final_build.log` y
`_local/gui/Stats_final_test.log`. La ventana PC y las pruebas aisladas no
acreditan gameplay ni ejecución en Vita.

Informes de las unidades previas, con cada función y sus excepciones:
[Background](RAMA_A_BACKGROUND.md), [GuiElement](RAMA_A_GUIELEMENT.md),
[GuiButton](RAMA_A_GUIBUTTON.md), [GuiTextBox](RAMA_A_GUITEXTBOX.md) y
[GuiWindow](RAMA_A_GUIWINDOW.md). Cada unidad pasó su build/ejecución PC y su
prueba antes de su commit local. No se hizo push.
