# Rama A — PlayerCustomizeMenu

Contrato PDB sin modificar. Inventario `pseudo_index.py ... --unit PlayerCustomizeMenu`, pseudocódigo y desensamblado en `_local/world-base/` y `_local/menus/` (ignorados por Git).

| Función indexada | Estado |
|---|---|
| PlayerCustomizeMenu | traducida @00707b80: flags, cuatro clases/paletas y sprite128×80 |
| ~PlayerCustomizeMenu | traducida como destructor por defecto; contenedores/shared_ptr/Sprite destruidos en el orden de miembros |
| step | traducida @00707e50: movimiento, parada, dos sorteos condicionales, partículas, posición y desconexión |
| drawCaravan | traducida @007081a0: oscuridad, color/alfa y dibujo SFML |
| reset | traducida @007082c0: UI y limpieza opcional de clases |
| close | traducida @00708400: open=false y inClassSelectionArea=false |
| addPlayer | traducida @00708480: factory, asignación de UI, clase/paleta e impulso inicial |
| addRandomizedPlayer | traducida @00708780: clase, trasfondo disponible, paleta, creación e inicialización |
| removePlayer | traducida @00708c40: mapa ID, inventario, vector de nivel, arma/pet, slot/clase y currentPlayer |
| menu | traducida @00709100: estado de selección, evitar paletas usadas, trasfondo y alta/baja |
| syncCaravanSprite | traducida @00709390: frame/tiempo/posición, coordenadas con offsets y estado de red |
| randomPalette | traducida @00709510: set ordenado0–3, exclusión del propio slot, un sorteo |

No se redefinieron getClasses/getPalettes existentes en inline_accessors.cpp. Se añadieron allí accessors de referencias/visibilidad/caravana/red, Entity::getLevel y Player::getCurrWeapon; sus comentarios indican el código consumidor que expande los campos. Level::getEntities recupera el vector usado por la eliminación inline. PlayerMenu::getPlayerClass @0071e430 es necesario para menu: devuelve availableClasses[unsigned(playerClass)%size], o Knight con diagnóstico si no hay clases. No es la lectura directa que aparenta el pseudocódigo. Otros métodos sin pseudocódigo en esta unidad siguen como stubs (spawnAll/checkSaveData/cancelAllMenus/exitLevel y consultas auxiliares).

field_writes.py, offset_name.py, call_args.py y asm contrastan: Entity.level+1A0, dx/dy+B4/B8; Unit.facingLeft+225 (Ghidra lo confunde con Player::_padding_); Player.canExitLevel+7F3, currWeapon+76B8 y unarmedWeapon+318. En PlayerMenu: choosingClass+335A, classChosen+335B, menuOpen+335D, availableBackgrounds+338C, selectedBackgroundIndex+33A8, playerBackground+33A4, playerPalette+33B0, randomClassAffinities+33BC. Los falsos accesos de Ghidra a checkboxSprite/lblConfigQuickSpells se sustituyen por esos campos reales. Se respetan cabeceras y vtables; no se tocaron GameEngine*.cpp ni Client.cpp.

Constantes recuperadas: TARGET_POSITION=56 (estaba TODO), posición inicial(97,256), atlas scenes/intro/caravan.png128×80, duración7, loop=true, origen(0,0). Movimiento: durante frames1–29, frame×**0x3c23d70b** (0.010000000707805157f; un ULP por encima de .01f); después0.3f. Detecta la parada en el tick posterior al cruce y entonces fija56. Rebote0.5 con sorteo%20; polvo con otro sorteo%3 solo si hay caravana/nivel y sigue moviéndose, posición(+114,+80), parámetro0/true. Caravana física(+68,+17). Sincronización Y+24. Fade de dibujo resta .01f, limita a0, RGB180−trunc(darkness×100), alfa trunc(alpha×255). Alta en caravana+(35,13), facingLeft=false, dx3.6/dy−2.

RNG: se comprobó el uniform_int global @0041cce0 con límites(0,INT_MAX); equivale a una extracción MT19937 enmascarada0x7fffffff. No se usa la distribución de libstdc++. Clase aleatoria%4; afinidades22/23/24 fuerzan clase4 **consumiendo igualmente el primer sorteo**. Trasfondo entre entradas desbloqueadas de Stats, con índice del resto convertido a byte como el exe; después sorteo de paleta sobre set ordenado. Los argumentos de nivel/coordenadas perdidos por Ghidra se recuperan de EDX/XMM2/XMM3. Los parámetros finales índice/bool optimizados fuera de los helpers se mantienen en la firma; se pasa slot y false, sin alterar las firmas generadas.

Límites: PlayerCustomizer::getNewPlayer/initializePlayer/updateAllPlayerStats, Player y PlayerMenu siguen como dependencias stub. El factory normal nulo se diagnostica en vez de desreferenciarlo. No se acredita todavía una partida New Game completa ni la creación real del personaje; las pruebas del menú cubren comportamiento independiente de esos helpers. Se añadieron guardas para referencias nulas/slots fuera de rango y set de paletas vacío, que el original suponía válidos. No se usan APIs de red/Steam; networked conserva sus ramas locales de presentación/entrada y no transmite datos.

Verificación: `pwsh tools/build_recon_pc.ps1` terminó en **OK:**; `pwsh tools/run_recon_tests.ps1 -Unit PlayerCustomizeMenu` pasó con asserts. Comprueba construcción, reset/close, selección de clase por módulo,31 ticks con orden RNG exacto, cruce/parada sin consumir RNG, sincronización con Sprite reflejado/offsets, colores y píxeles dibujados, hidden, paletas que excluyen otros slots, baja vacía, altas en slot ocupado y retornos de menú. Textura sintética, sin recursos comerciales en Git. Logs `_local/world-base/PlayerCustomizeMenu_build.log` y `PlayerCustomizeMenu_test.log`.

`pwsh tools/build_recon_pc.ps1 -Run` también terminó con **exit code: 0**, y recon.log registró50 funciones pendientes alcanzadas; log `_local/world-base/PlayerCustomizeMenu_desktop_run.log`. Alcanzó Client::step/draw y GameEngine::step (stub). El controlador localizó la ventana pero su aprobación de captura expiró, por lo que no se acredita observación visual del menú nativo ni gameplay. La ejecución anterior dentro del sandbox no exponía su ventana y se descartó; el resultado citado es la repetición en escritorio. Ninguna prueba acredita gameplay Vita.
