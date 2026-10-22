# Traducción de unidades — rama A

Fecha: 2026-10-21. Trabajo local en `rama-a`; no se hace push.
Las cabeceras generadas en `recon/include` se conservan sin editar.

## Lista de tareas

| Tarea | Estado |
|---|---|
| 1. Log | Terminada previamente, commit `cc90031` |
| 2. Config | Traducida y verificada en PC |
| 3. Tablas de identificadores y Stats | Pendiente |
| 4. Input | Índice completo atendido; compilación/arranque PC y suite repetible pasan. [Informe por función y exclusiones](RAMA_A_INPUT.md) |
| 5. Sprite | Pendiente |
| 6. VFont | Pendiente |
| 7. ResourceManager (bloque inicial solicitado) | Nueve funciones traducidas y verificadas en PC con `data.vra`; catálogos y procesadores profundos pendientes. [Informe por función](RAMA_A_RESOURCEMANAGER_BLOQUE1.md) |
| 8. Constantes de una unidad | Terminada para Config: cuatro constantes recuperadas |

## Config: funciones y evidencia

| Función de `--unit Config` | Estado |
|---|---|
| `Config::load` — `00577110` | Traducida: preferencia por INI, migración del legado y selección del idioma por defecto |
| `Config::loadIni` — `005777d0` | Traducida: conserva orden, claves, valores por defecto y llamadas de volumen/DirectInput |
| `Config::loadUpgrade` — `00576f80` | Traducida: versión 34; tamaños y argumentos comprobados con `call_args.py` |
| `Config::saveIni` — `00578860` | Traducida: secciones y claves en el orden del original |
| `Config::setLanguage` — `00579aa0` | Traducida con adaptación portable de catálogos externos `.mo` |
| `Config::setWorkshopLocalization` — `0057a350` | Cuerpo vacío con comentario: Steam/Workshop excluido por instrucción del usuario |
| `Config::updateGamma` — `00579570` | Traducida: uniform `gamma` del shader `postprocessing.frag` |
| `Config::updateWindowSize` — `005796a0` | Traducida: limita tamaño al escritorio, crea ventana SFML, desactiva repetición y aplica VSync |
| `Config::~Config` — `00576f60` | Traducida mediante destructor por defecto de STL |
| `estream::estream` — `005834b0` | Traducida con `std::fstream` binario y creación opcional |
| `estream::~estream` — `00577740` | El destructor implícito de la cabecera hace la misma destrucción de `path` y `file`; no se declara uno adicional ni se edita la cabecera |
| `operator>>(estream&, int&)` | Traducida: lectura binaria de cuatro bytes |
| `type_info::name` | CRT de MSVC; se usa el RTTI de C++ del compilador. No se reconstruye el objeto interno de MSVC |

Además: `Config::Config()` se recupera del constructor expandido en `main` y de los valores proporcionados por el usuario. `Config::save()` es un wrapper de `saveIni` (inferencia; no tiene cuerpo independiente en el índice). Los helpers inline de `estream` (`close`, `is_open`, `skip`, `tellg`, `readInto`) también están implementados.

### Constantes recuperadas

- `FILE_FORMAT_VERSION = 35`: comparación `0x23` en `Config::load`; se migra el formato `34`.
- `LEGACY_CONFIG_FILE_NAME = "vagante.cfg"`: inicializador `00402a00`.
- `CONFIG_DIR_PATH = "./config"`: inicializador `00402a20`.
- `CONFIG_FILE_NAME = "config.ini"`: inicializador `00402a40`.
- `viewZoom` conserva bits `0x3fc5d174`. Constructor: `1280×720`; valores ausentes del INI: `1024×768`.
- Los índices de las claves `player0`…`player3` y los volúmenes individuales se verificaron en desensamblado; Ghidra había perdido los registros.

### Adaptaciones y dudas

- El segundo `int` de `loadUpgrade` no se utiliza. En el ejecutable optimizado el llamador pasa un registro con la dirección del stream. Se conserva la firma PDB y se ignora ese argumento.
- No hay Boost.Locale en la capa actual. La adaptación STL instala un facet con el catálogo `.mo` externo y conserva las consultas singulares. La generación de reglas de plural y todas las facetas de formato de Boost.Locale no se han reconstruido; si el host no tiene el locale solicitado se usa el formato clásico.
- El icono embebido `sfml_icon` es un recurso del juego que no está declarado en las cabeceras. Se admite un `icon.png` externo del usuario; no se incorpora la imagen comercial al repositorio.
- El fullscreen del original usa estilo SFML `0` (ventana sin bordes al tamaño del escritorio), no el estilo exclusivo de pantalla completa.

### Verificación

- `pwsh tools/build_recon_pc.ps1 -Run`: compila y enlaza sin errores; ejecutable nuevo, ventana SFML creada; cierre de la ventana de prueba por `CloseMainWindow`; **exit code: 0**.
- `recon.log`: 26 funciones sin traducir alcanzadas; ninguna de Config.
- `pwsh tools/run_recon_tests.ps1 -Unit Config`: pasó constructor/bit exacto del zoom, valores ausentes del INI, round-trip de todos los jugadores, rechazo de INI mal formado/duplicado, consulta de catálogo `.mo` y helpers binarios de `estream`.
- Los temporales de GCC se dirigen a `_local/gcc-tmp` mediante `BASH_ENV`; no se alteran el sistema de build ni la instalación de MSYS2.
- No se validó en hardware Vita.
