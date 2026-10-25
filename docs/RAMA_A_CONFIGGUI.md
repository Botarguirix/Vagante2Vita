# Rama A — ConfigGui

Cabeceras PDB intactas. Se listó `pseudo_index.py ... --unit ConfigGui` y se contrastaron los argumentos perdidos con `call_args.py`, las instrucciones SSE y los textos del PE. Fuentes Ghidra: 00583980–0058b92e; evidencia local en `_local/menus/ConfigGui_*.txt` y `.asm`.

| Función indexada | Estado |
|---|---|
| ConfigGui::ConfigGui | traducida; se completa el constructor previamente integrado. Se definen juntas ambas sobrecargas declaradas; copia por miembros generada por C++ |
| ConfigGui::choosePlayer | traducida |
| ConfigGui::cleanUp | traducida |
| ConfigGui::down | traducida |
| ConfigGui::draw | traducida |
| ConfigGui::initialize | traducida |
| ConfigGui::isOpen | traducida previamente en inline_accessors.cpp; revisada, no se duplica |
| ConfigGui::left | traducida |
| ConfigGui::right | traducida |
| ConfigGui::selectIndex | traducida |
| ConfigGui::setInGameState | traducida |
| ConfigGui::step | traducida |
| ConfigGui::stepControls | traducida |
| ConfigGui::stepMain | traducida |
| ConfigGui::stepOptions | traducida |
| ConfigGui::toggle | traducida |
| ConfigGui::up | traducida |
| ConfigGui::updateButtonStates | traducida |
| ConfigGui::updateInputController | traducida, sin llamadas a Steam |
| ConfigGui::updateInputSelection | traducida |
| lambda_bc2e6b26c8fae3bcc536e52f1356e2fd::operator() | traducida como cell en initialize; inserta la última entrada de controles |
| lambda_c9fa69f79c54fd3ec4836dced343db48::operator() | traducida como mouseHover; misma lógica expandida en las otras pantallas |
| lambda_d710871c20c6fd5362d9ff00cbff610a::operator() | traducida como addOption; posiciones, celdas dobles y salto de columna |
| lambda_e9f140f1b56e46653dc0ae2921fa74e2::operator() | traducida como title en draw |
| Input::ControllerIdentifier::~ControllerIdentifier | destructor implícito C++ de std::string; cabecera sin destructor explícito |
| SteamInternal_Init_SteamController | cuerpo vacío con comentario, conforme a exclusión de Steam; no enlaza SDK |

Auxiliares: GridPosition constructor/comparador lexicográfico x/y, getConfig y destructor (cleanUp). Se conservan los helpers previamente integrados actionPress/actionRelease/getScreen/setScreen/doRunCredits. resized y mouseAction no tienen pseudocódigo independiente en --unit y siguen como stubs; no se inventan sus cuerpos.

Constantes recuperadas: ANIMATE_FRAMES=10; cursor(139,110), fondo32×32; seis botones(196,92+20*i),120×16; opciones con origen52, fila20, columnas196/376 y centro256; controles por jugador desde43, paso112; 15 acciones, filas18 desde71, binds(290/386) de60×17; slider volumen sqrt(volumen*.01) y aplicación valor*100*valor; gamma/zoom/minimapa valor+1. Las columnas y celdas mantienen insert-first de std::map. Show Debug Info arranca en false incluso si debugLevelGen era true: `push 0` original.

Orden: step actualiza habilitaciones, temporizadores, frame, direcciones, ratón y pantalla antes de resolver selección/cursor/afterStep. La conversión del ratón usa la vista activa de RenderWindow, sobrecarga sin View, confirmada en 00587290. Se conserva el dibujo del diálogo antes de actualizar su alfa. No se añaden llamadas RNG: la muestra SFX consume un MT enmascarado0x7fffffff módulo4 y después llama a playSound; los otros recorridos no sortean. Los cuatro sonidos y textos se leen del ejecutable.

Los accesos privados expandidos se expresan por las APIs públicas. GuiSelect::setEntries se contrastó contra su cuerpo: copia entries, resize selected y altura=lineH*tamaño. GuiSelect conserva la firma PDB de cuatro floats, pero el constructor compilado solo usa x/y; sus dimensiones retenidas no se inventan. Se mantiene EJoystick::updateDirectInput como flag existente, sin añadir APIs DirectInput/XInput/Windows. El overlay de reasignación Steam queda vacío.

## Verificación

Build integrado desde la raíz: `pwsh tools/build_recon_pc.ps1 -Run` regeneró stubs, compiló sin errores, ejecutó vagante.exe y cerró normalmente: **exit code: 0**, **58** funciones aún sin traducir alcanzadas, listadas en `_local/build-recon-pc/recon.log`. Log completo `_local/menus/ConfigGui_final_build.log` y diagnóstico `_local/build-recon-pc.log`.

`pwsh tools/run_recon_tests.ps1 -Unit ConfigGui`: prueba de estructura (6/20/57 elementos, 30 binds), mapas/celdas compartidas, deduplicación de direcciones, selección por jugador, apertura y temporizadores, vista de ratón, pantallas y diálogo de salida, estados de menú/red, cambio de dispositivo, filtrado de perfiles, habilitación durante binding, reasignación individual/completa, dibujo offscreen y limpieza. Log `_local/menus/ConfigGui_test.log`.

También se repitieron y pasaron sobre el build integrado las pruebas Tween, PaletteDatabase, MainMenu, Credits y ProgressionGui. Sus logs son `_local/menus/<Unidad>_integrated_test.log`. Los bloqueos históricos de Client ya fueron corregidos en los commits de su autor; aquí no se modificó Client ni el build.

## Dudas y límites

GuiSlider, GuiSelect, GuiCheckbox y GuiLabel siguen sin traducir; GuiWindow::setAlpha también sigue como stub. Sus llamadas públicas se conservan. Esto impide acreditar el ajuste interactivo de valores, la selección real de presets y el render completo de opciones. El test no sustituye esos stubs ni valida por simulación sus valores. GuiCursor ya tiene implementación integrada.

Se preserva la limpieza original de los tres vectores propietarios y el diálogo; los vectores auxiliares guardan alias. Repetir initialize sobre la misma instancia conserva alias antiguos en el cuerpo observado: no se ha corregido sin evidencia adicional; esa ruta queda sin verificar. La copia por miembros declarada en la cabecera tampoco se usa en las pruebas.

Los helpers no indexados resized/mouseAction continúan pendientes. No se probó Vita ni gameplay y no se generó VPK. Sin push ni recursos comerciales en Git.
