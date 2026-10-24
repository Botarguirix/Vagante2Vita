# Rama A — ProgressionGui

Contrato: cabeceras PDB sin modificar; pseudocódigo 004d6a20–004db680 y comparadores expandidos en 004ddc30/004ddd30. Se ejecutó `pseudo_index.py ... --unit ProgressionGui`.

| Función indexada | Estado |
|---|---|
| ProgressionGui::ProgressionGui | traducida |
| ProgressionGui::~ProgressionGui | traducida |
| ProgressionGui::handleInput | traducida |
| ProgressionGui::pushAnimation | traducida |
| ProgressionGui::step | traducida |
| ProgressionGui::doButton | traducida |
| ProgressionGui::doCard | traducida |
| ProgressionGui::updateStates | traducida |
| ProgressionGui::drawMenu | traducida |
| ProgressionGui::drawItems | traducida |
| ProgressionGui::drawGameOver | traducida |
| lambda_289710b41a7c54313d998fdb670cf7ef::operator() | traducida como pageText en step |
| lambda_808af787e8551b8a2193d684559ab8e3::operator() | traducida como rebuild en step |
| lambda_bd6b2e7fae9928fa28aea25ec069e992::operator() | traducida como selectionKey |
| lambda_f35ebac077c005efee4fc56f5bad9cfe::operator() | traducida como cardText |
| GuiItem::GuiItem | construcción implícita de Sprite, vectores y Vector2; la cabecera no declara constructor. Las asignaciones originales ponen a cero la memoria antes de construir: GuiItem{} reproduce sus escalares |
| PrettyText::~PrettyText | destructor implícito del vector STL |
| Unlockable::Unlockable | copia/movimiento implícitos de los miembros de la cabecera |
| Unlockable::operator= | asignación implícita de los miembros |
| Unlockable::~Unlockable | destructor implícito de cadenas y std::function |

Auxiliares expandidos: popAnimation y guiItemContainsPos. initGameOver/stepGameOver no tienen entrada en esta unidad; continúan como stubs pendientes de recuperar sus cuerpos expandidos en Client, sin inventar una secuencia.

Constantes recuperadas: CARD_W=98, CARD_H=64; cuadrícula 147×74 desde (173,102), 12 tarjetas/página; botones desde (194,44), paso84; entrada500 con 1/15 por tick; salida50/tick hasta60. Flips: pi=3.1415927410125732, pi/2=1.5707963705062866, 3pi/2=4.71238899230957, ángulo/tick .05235988274216652 y período300 tras espera120. Colores y umbrales del texto, ancho87 y separación14, conservados. Sin llamadas RNG.

Se contrastaron las llamadas doCard y getProgressionInterval con `call_args.py`, y los campos/curvas de step con instrucciones SSE: Ghidra confunde frame, animationProgress y cardType. Orden de clasificación: unlocked (según modo), tipo ascendente, sin condición primero, progressionLevel ascendente; sin desempate artificial por nombre.

Verificación: `pwsh tools/build_recon_pc.ps1 -Run` en `_local/menus/validation`, copia aislada sin el Client.cpp ajeno que no compila. Regeneró stubs, compiló, ejecutó y cerró la ventana nativa: exit code: 0; 42 funciones sin traducir alcanzadas. Log local: `_local/menus/ProgressionGui_build.log`. `run_recon_tests.ps1 -Unit ProgressionGui`: passed, incluyendo 25 desbloqueos/3 páginas, ambas clasificaciones, entrada de 4 jugadores, pulsación/liberación, límites de ratón, textos/colores, animaciones y dibujo offscreen. Log `_local/menus/ProgressionGui_test.log`.

Límites: el texto sin espacios que supera87 se deja con diagnóstico al agotar divisiones, evitando repetir indefinidamente el fallo de Ghidra. El flujo completo de game over continúa dependiendo de Client. No se verificó gameplay Vita. La verificación integrada queda pendiente mientras Client.cpp tenga accesos privados inválidos; no se modificó ese archivo.
