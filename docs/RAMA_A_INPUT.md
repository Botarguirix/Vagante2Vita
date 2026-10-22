# Tarea 4 — Input

Fecha: 2026-10-21. Rama `rama-a`; commit local, sin push.
Fuentes: contrato PDB sin editar, pseudocódigo del índice Input y desensamblado
para recuperar argumentos y comparaciones omitidos por Ghidra.

Las **57 entradas** de `--unit Input` están atendidas: **53 traducidas** (incluidas
las dos lambdas de lógica), **dos funciones Steam excluidas** por las reglas comunes
y **dos entradas de construcción/destrucción implícitas** del compilador.
Todo el C++ de esta entrega está en `recon/src/Input.cpp`.

## Informe por función

| Función | Estado / motivo | Dirección Ghidra |
|---|---|---|
| `<lambda_586d774fca34af834a456731902debe3>::operator()` | Traducida en controllerSection: «Controller N» y clave de sección. | `00600140` |
| `<lambda_e82d52b5b2b44ba60a4b4ab188f79f82>::operator()` | Traducida en randomProfileFilename: tabla Markov y cuatro extracciones del RNG. | `00604970` |
| `ControlScheme::ControlScheme` | Implícita: copia y movimiento generados por el compilador; la cabecera no declara constructores propios. | `00608310, 00608700` |
| `ControlScheme::copyToProfile` | Traducida. | `00605cf0` |
| `ControlScheme::loadFromProfile` | Traducida. | `00605ea0` |
| `ControlScheme::~ControlScheme` | Implícita: destrucción STL generada por el compilador; no se modifica la cabecera. | `005fd960` |
| `Input::Input` | Traducida. | `005f8cc0` |
| `Input::cancelBinding` | Traducida. | `005fd9b0` |
| `Input::countUsedKeys` | Traducida. | `00603670` |
| `Input::deleteSelectedText` | Traducida. | `00603800` |
| `Input::enteringTextKeyEvent` | Traducida. | `005ff260` |
| `Input::generateAndLoadDefaultProfiles` | Traducida. | `005fe320` |
| `Input::getActionHandle` | Traducida. | `00603f20` |
| `Input::getBindFromDescriptor` | Traducida. | `006021e0` |
| `Input::getBindName` | Traducida. | `005fa0a0` |
| `Input::getControllerButtonSprite` | Traducida. | `00602580` |
| `Input::getControlsType` | Traducida. | `00603440` |
| `Input::getDescriptorFromBind` | Traducida. | `00601ed0` |
| `Input::getDescriptors` | Traducida. | `00601fd0` |
| `Input::getKeyName` | Traducida. | `00601860` |
| `Input::getNetState` | Traducida. | `005fa040` |
| `Input::getSteamSprite` | Excluida por regla común: sin API de Steam; devuelve nullptr. | `00602f00` |
| `Input::getXInputBindName` | Traducida. | `005fa810` |
| `Input::getXInputButtonSpriteId` | Traducida. | `006032a0` |
| `Input::initControllerSprites` | Traducida. | `005fca70` |
| `Input::initSteamController` | Excluida por regla común: cuerpo vacío con comentario, sin API de Steam. | `00602910` |
| `Input::inputChar` | Traducida. | `005ff5d0` |
| `Input::inputText` | Traducida. | `005ff810` |
| `Input::isHeld` | Traducida. | `005fb3f0` |
| `Input::isKeyHeld` | Traducida. | `005fb4f0` |
| `Input::isKeyPressed` | Traducida. | `005fb520` |
| `Input::isKeyReleased` | Traducida. | `005fb570` |
| `Input::isPressed` | Traducida. | `005fb430` |
| `Input::isReleased` | Traducida. | `005fb490` |
| `Input::joystickConnectEvent` | Traducida. | `005fd080` |
| `Input::joystickDisconnectEvent` | Traducida. | `005fd1b0` |
| `Input::load` | Traducida. | `006002b0` |
| `Input::loadDefaults` | Traducida. | `005fda50` |
| `Input::loadLegacyFormat` | Traducida. | `00601350` |
| `Input::loadProfiles` | Traducida. | `00600e20` |
| `Input::reset` | Traducida. | `005fd2e0` |
| `Input::save` | Traducida. | `005ff840` |
| `Input::saveProfiles` | Traducida. | `00600db0` |
| `Input::setEnteringText` | Traducida. | `005ff1f0` |
| `Input::setTextCursorToEnd` | Traducida. | `005ff1d0` |
| `Input::setUsingJoystick` | Traducida. | `005fee40` |
| `Input::step` | Traducida. | `005fc4e0` |
| `Input::stepBindController` | Traducida. | `005fb5b0` |
| `Input::stepController` | Traducida. | `005fbf30` |
| `Input::tryCopy` | Traducida. | `00603950` |
| `Input::tryPaste` | Traducida. | `00603c60` |
| `Input::tryToUseAController` | Traducida. | `00602ba0` |
| `Input::updateActionNames` | Traducida. | `005f9140` |
| `Input::updateGamepadStatus` | Traducida. | `005fc600` |
| `Input::~Input` | Traducida. | `005f9050` |
| `InputProfile::loadProfile` | Traducida. | `006053f0` |
| `InputProfile::saveProfile` | Traducida. | `00604020` |

## Implementación y constantes

- Teclado por `sf::Keyboard`; botones, ejes, identificación y enumeración por
  `sf::Joystick`. Portapapeles por `sf::Clipboard`, con conversiones de `sf::String`.
  No se añaden llamadas Win32, DirectInput, XInput ni Steam.
- El archivo Input original usa accesos expandidos a `EJoystick::m_states` y
  `m_joysticks`. Esas lecturas se adaptan a SFML. **EJoystick y sus llamadas desde
  main/Client no se modifican**; su implementación sigue siendo trabajo de otra unidad.
  Los métodos cuyos nombres contienen XInput solo traducen etiquetas/glifos.
- `MAX_PLAYERS=4`, `BIND_TIME=20`. `INPUT_FILE_FORMAT_VERSION=7` ya estaba recuperada
  en statics; se lee también versión 6, que omite WALK. Cada acción serializada ocupa
  24 bytes; CHAT se guarda una vez antes de los cuatro slots.
- `ACTION_HANDLES`: 15 textos del inicializador `00403200`, en el orden del enum:
  left, right, down, up, jump, melee, range, skill, swap, walk, playermenu, pause,
  interact, emote, chat.
- Reasignación: 20 ticks de una candidata estable, deadzone de captura 40, umbral
  de activación 50 (cero para specialL2R2). La tecla/botón recién asignado se suprime
  hasta soltarlo. Los binds alternativos solo se aplican al movimiento (acciones 0–3).
- Se conservan los cuatro perfiles y su orden: Default Keyboard, Classic Keyboard,
  Keyboard Home-Row, Default (XInput). El formato usa `[Controller 0]`…`[Controller 3]`,
  descriptores separados por ` | `, nombres SFML históricos y `[botón]` / `[+eje]` / `[-eje]`.
  Se contrastó también el formato con el input.ini local original, sin modificarlo.
- El portapapeles usa SFML. Los cursores conservan índices de bytes del std::string
  UTF-8, como el original. Pegar inserta en el cursor activo sin borrar la selección.
- Nombres aleatorios: `profile_` y cuatro letras, más `.ini`. Se conserva la tabla
  Markov (incluidas sus particularidades para s/u) y cuatro extracciones de MT19937
  por intento. `_intDistribution` original contiene `[0, INT_MAX]` en `00b81f88`;
  su implementación `0041cce0` toma el resto módulo 0x80000000. Se usa esa operación
  explícita, evitando diferencias entre uniform_int_distribution de MSVC y libstdc++.
- Sprites de controles: tiles de 32×32, escala 0.5. Argumentos de constructor
  recuperados con `call_args.py Input::initControllerSprites --callee Sprite::Sprite`.
- El ensamblado de getBindFromDescriptor compara bytes 0x5b/0x5d/0x2b/0x2d,
  es decir `[`, `]`, `+`, `-`; Ghidra los había expresado erróneamente como bools.

## Dudas y adaptaciones explícitas

- `Input::reset(int)` y `loadDefaults(int)` ignoran el argumento en el cuerpo
  optimizado y afectan a los cuatro slots. Se conserva la firma PDB y ese comportamiento.
- `getNetState(unsigned)` original empaqueta siempre **slot cero**, confirmado en
  `005fa044`/`005fa055`; no se corrige silenciosamente. Es una conversión local de
  bits, sin transporte ni llamadas de red.
- El lector original de InputProfile solo llena bindings, mientras loadFromProfile
  intenta leer secondaryBindings para perfiles de mando. Se protege el acceso cuando
  ese vector está vacío, usando un bind alternativo no asignado. También se comprueban
  índices de teclado/ejes antes de consultar SFML para evitar accesos inválidos.
- La familia del mando se determina por IDs SFML: se conservan los pares Sony y Xbox
  One del original; la antigua bandera raw xinput se sustituye por vendor Microsoft
  para etiquetas Xbox 360. No se llama a ninguna API XInput para ello.
- La correspondencia entre los IDs históricos de los perfiles y botones físicos de
  cada dispositivo SFML **no se ha probado con un mando físico**. Tampoco se han
  validado visualmente glifos: dependen de Sprite, que sigue pendiente en otra tarea.
- `initSteamController` queda vacío y `getSteamSprite` devuelve nullptr. Las ramas
  de consulta Steam se omiten; getBindName conserva «ERROR» cuando Steam no está disponible.
- ControlScheme usa copia, movimiento y destructor implícitos de STL. Se implementan
  como dependencias todos los constructores declarados de ActionBind, InputProfile
  y ControllerIdentifier. No se añaden declaraciones a las cabeceras generadas.
- Los helpers declarados `stepSwitchedTimers`, `rebindAll`, `rebindAction`,
  `saveLegacyFormat` y `tryUpgrade` no aparecen en la lista solicitada ni tienen
  pseudocódigo indexado; se conservan sus stubs, sin inventar sus reglas.

## Verificación ejecutada

- `pwsh tools/build_recon_pc.ps1 -Run`: regeneración de stubs, compilación/enlace MSYS2
  sin errores, ventana SFML nativa y cierre normal: **exit code: 0**.
- `recon.log`: **60** funciones pendientes de otras unidades alcanzadas. Ninguna de
  Input, ControlScheme, InputProfile o ActionBind. EJoystick conserva sus stubs externos.
- `pwsh tools/run_recon_tests.ps1 -Unit Input`: **exit 0**. Ejecutable de la suite
  repetido una segunda vez: **exit 0**, con un directorio distinto de fixtures por corrida.
- Pruebas: estados actual/anterior y flancos, slots habilitados, captura estable de
  20 ticks y supresión hasta soltar, snapshot con polling SFML, 101 nombres de teclas,
  descriptores, textos/glifos por IDs, selección y entrada UTF-8, INI de cuatro slots,
  perfiles de teclado/mando, recarga sin duplicar perfiles, consumo de cuatro RNG,
  rechazo de INI mal formado y lectura de formatos binarios 6/7 (1320/1416 bytes).
- Los fixtures y registros viven en `_local`; las pruebas no alteran la configuración
  del usuario ni el portapapeles del sistema. No se incorporan recursos comerciales.
- Evidencia: `_local/input-build-run.log`, `_local/input-tests.log`,
  `_local/input-tests-repeat.log`, `_local/build-recon-pc/recon.log` y
  `_local/build-recon-pc/logs/log.log`. Evidencia de argumentos en `_local/input/`.
- Ejecutable PC probado, SHA-256:
  `38715b2c1f9dc7f7e6fa26731c983977b43eca59f4f43d1d76fa4c44625577f5`.

Esta verificación cubre software y arranque PC. No acredita gameplay, mando físico,
portapapeles interactivo, glifos renderizados, Vita3K ni hardware Vita.
