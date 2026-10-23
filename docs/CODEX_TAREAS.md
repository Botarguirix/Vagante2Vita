# Tareas para Codex (rama A)

Cada tarea es independiente y se puede pegar tal cual en Codex. Todas trabajan en
`C:\Users\Andy PC\Documents\Chatgpt\Vagante2Vita`, rama `rama-a`, y **solo** crean
o editan los archivos que la tarea indica (Claude trabaja en el resto).

## Reglas comunes (incluir siempre)

```
Proyecto: reconstrucción en C++ de Vagante para un port a PS Vita (rama A).
Carpeta: C:\Users\Andy PC\Documents\Chatgpt\Vagante2Vita, rama git rama-a. No hagas push.
Git: usa C:\Users\Andy PC\AppData\Local\GitHubDesktop\app-3.6.7\resources\app\git\cmd\git.exe

Fuentes de verdad, en este orden:
1. recon/include/*.h: cabeceras generadas desde vagante.pdb. Son el contrato: layout,
   nombres, firmas y vtables exactos. NO las edites (se regeneran). Si una firma parece
   mal, anótalo en el informe en vez de cambiarla.
2. Pseudocódigo de Ghidra, una función por archivo en
   C:\Users\Andy PC\Documents\Chatgpt\VaganteVita\_local\decompiled
   Búscalo por nombre con:
   python -I tools/pseudo_index.py "C:\Users\Andy PC\Documents\Chatgpt\VaganteVita\_local\decompiled" --show Clase::metodo
   y lista las funciones de una unidad con --unit <Unidad>.
3. recon/stubs/statics.cpp: constantes leídas del ejecutable. Las marcadas
   "TODO value" no estaban en la imagen: búscalas como valores inmediatos en el pseudocódigo.

Alcance: la tarea está terminada solo cuando están traducidas TODAS las funciones que lista
  python -I tools/pseudo_index.py "<carpeta decompiled>" --unit <Unidad>
(constructores, destructores y estáticos incluidos), salvo las marcadas "-" (sin pseudocódigo)
y las que el compilador generó solo (`scalar deleting destructor`, lambdas internas de la STL).
En el informe final, lista cada función con "traducida" o el motivo por el que no.

Cómo traducir:
- Escribe C++ legible y fiel al comportamiento, no una copia literal del pseudocódigo:
  quita los artefactos de MSVC (std::_Tree, _Mypair, refcounts manuales de shared_ptr,
  __security_cookie, destructores inline de std::string, etc.) y usa la STL y SFML 2.5 normales.
- Conserva el orden de las operaciones, las constantes y las llamadas al RNG exactamente:
  la generación de niveles depende de ello.
- Todas las sobrecargas de un método van juntas (el generador deja de crear stubs
  para un nombre en cuanto ve su definición en recon/src).
- No uses APIs de Windows ni de Steam. Red/Steam: deja el cuerpo vacío con un comentario.

Verificación (obligatoria antes de terminar), desde PowerShell 7 en la carpeta del repo:
  pwsh tools/build_recon_pc.ps1 -Run
Regenera los stubs, compila con MSYS2 y ejecuta vagante.exe. Debe compilar sin errores,
terminar con "exit code: 0" y recon.log lista las funciones aún sin traducir que se alcanzaron.
Los errores de compilación están en _local/build-recon-pc.log. Haz commit solo de tus archivos, con un mensaje
"rama-a - traducir <Unidad>". Informe final: funciones traducidas, dudas y constantes
recuperadas.
```

Herramienta extra: cuando Ghidra pierda argumentos (bools, ints o textos pasados en
registros), mira el desensamblado real antes de cada llamada:
  python -I tools/call_args.py Clase::metodo --callee Otra::funcion --before 8
Los textos en direcciones (p. ej. `pushl $0xafbfdc`) se leen con:
  python -I -c "import sys;sys.path.insert(0,'tools');from pdb_stubs import PE;print(PE(r'F:\SteamLibrary\steamapps\common\vagante\vagante.exe').cstring(0xafbfdc))"

Patrón de log del original: `if (Log::reportingLevel() >= NIVEL) Log().get(NIVEL) << ...`.
En el C++ escríbelo con la macro `VLOG(NIVEL) << ...` (defínela al inicio del .cpp como en
recon/src/main.cpp). Muchos accesos a campos privados en el pseudocódigo son getters/setters
inline expandidos: usa el método público equivalente de la cabecera.

## Tareas

| # | Estado | Unidad | Archivo | Notas |
|---|---|---|---|---|
| 1 | hecha | `Log` | `recon/src/Log.cpp` | Ya está `getConditional`. Faltan `Log::Log`, `~Log`, `get`, `init` (fija `reportingLevel` en 10), `closeLogAndGetBody` y los estáticos `toString`, `fromString`, `reportingLevel`, `ensureLogDirectoryExists`. |
| 2 | hecha | `Config` | `recon/src/Config.cpp` | Incluye `Config::Config()`: el original lo expandió dentro de `main`. Valores: `musicVolume=50`, `sfxVolume=50`, `viewZoom=0x3FC5D174` (1.5454545f), `particlesEnabled`, `useVertexArrays`, `sendStatistics` y `loadFromArchive` en `true`, `fullScreen=false`, `enableMenuToolTips`, `enableInGameTooltips` y `verticalSync` en `true`, `cumulativeCombatText` y `steamBigPicture` en `false`, `windowSize=(1280,720)`, flags 0x20–0x23 en `false`, `gamma=1`, `debugLevelGen=false`, `drawNetInformation=true`, `forcePerfectPixelScaling=false`, `firstTimePlaying=false`, `supportDirectInput=false`, `minimapScale=1`, `language=15`, `gameModes.resize(1,false)`, `quickCastEnabled.resize(4,false)`, `quickSwapEnabled.resize(4,false)`. `updateWindowSize` crea la ventana SFML. |
| 3 | pendiente | `Entity::initEntityStringIdentifiers`, `ItemAttribute::initAttributeStrings`, `Skill::initSkillStringIdentifiers`, `Stats` | `recon/src/Strings.cpp` (las tres primeras) y `recon/src/Stats.cpp` | Tablas de nombres; las llama `main` al arrancar. |
| 4 | hecha | `Input` | `recon/src/Input.cpp` | Sin DirectInput: teclado y joystick por SFML. |
| 5 | pendiente | `Sprite` | `recon/src/Sprite.cpp` | Animaciones por atlas; la usan casi todas las entidades. |
| 6 | pendiente | `VFont` | `recon/src/VFont.cpp` | Fuente bitmap del juego. |
| 7 | bloque 1 hecho | `ResourceManager` | `recon/src/ResourceManager.cpp` | Grande (~26 000 líneas de pseudocódigo). Carga de `data.vra`; el formato está documentado en `tools/vra.py`. Puede hacerse en varias entregas: primero `initialize`, `finishInitialization`, `initThreadsAreDone`, `initTaskProgress`, `isInitialized`, `setConfig`, `getNewLoadingTexture`, `cleanUp`. |
| 7b | pendiente | `ResourceManager` bloque 2 | `recon/src/ResourceManager_*.cpp` | La carga real de recursos: `queueLoadTextureTasks`, `queueLoadSoundTasks`, `runTasks`, los procesadores de tareas, `getTexture`/`getSound`/`getFont`/`playSound`, música y volúmenes (`setMusicVolume`, `setGlobalSfxVolume`). Objetivo visible: la barra de carga avanza mientras se cargan las 2 395 texturas y 304 sonidos de `data.vra`. |
| 8 | pendiente | Constantes | `recon/src/constants_<unidad>.cpp` | Recuperar los valores "TODO value" de `statics.cpp` de una unidad, buscándolos en su pseudocódigo. |

Claude se encarga de: `main` (hecho), `GameEngine`, `Client`, el sistema de build, la capa SFML para Vita y la revisión.
