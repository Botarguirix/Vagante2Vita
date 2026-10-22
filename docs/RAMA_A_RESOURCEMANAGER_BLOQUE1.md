# ResourceManager — primer bloque de la tarea 7

Trabajo local en `rama-a`, 2026-10-21. Sin push. Alcance de esta entrega:
las nueve funciones solicitadas y sus dependencias de lectura del archivo del usuario.
La unidad completa sigue parcial; no se declara terminado el cargador de todos los recursos.
Las cabeceras PDB no se han editado. No se incluyen recursos comerciales en Git ni VPK.

## Funciones solicitadas

| Función | Estado y evidencia |
|---|---|
| `initialize` | Traducida, `0073c290`: archivo, orden de las tareas, contadores y trabajadores alternando preferencia gráfica/sonido. `call_args.py` confirma los tipos 1, 14, 15, 16, 17 y atributos de las tareas. |
| `finishInitialization` | Traducida, `0073c6c0`: join, procesamiento de atlas, shader, orden de nombres y liberación de bloques temporales. El modo incremental puede devolver antes de marcar initialized. |
| `initThreadsAreDone` | Traducida, `0074ecc0`: exige `numThreads > 0` y `threadsCompleted >= numThreads`. |
| `initTaskProgress` | Traducida: cociente float expandido dentro de `main`, sin clamp ni caso especial de divisor cero. |
| `isInitialized` | Traducida: lectura atómica expandida dentro de `main`. |
| `setConfig` | Traducida: asignación expandida dentro de `main`. |
| `getNewLoadingTexture` | Traducida, `0073c0a0`: try-lock de texturas y luego imágenes, caché y subida de `gui/loading.png`. |
| `cancelInitialization` | Traducida, `0073ca70`: vacía las tres colas bajo M_TASKS. No simula finalización ni detiene tareas ya tomadas. |
| `cleanUp` | Traducida, `0074a520`: join y destrucción de los recursos que posee el original, con el mismo orden. |

## Dependencias de este bloque

| Función | Estado |
|---|---|
| `Task::Task` | Traducida, incluidos sus seis argumentos y `attempts=1`. La copia y destrucción de strings son las implícitas de C++. |
| `_lock`, `_try_lock`, `_unlock` | Traducidas usando los 18 mutexes del contrato. |
| `queueTask` (ambas sobrecargas) | Traducidas, `0074fd90`/`0074fe60`: rutas de las colas y checksum con suma de hashes FNV-1a de los tres textos. |
| `getNextTask` | Traducida, `0074fb30`: preferencia G→S→P o S→G→P. |
| `requeueTask` | Traducida, `00750110`: vuelve a la cola de procesamiento, máximo cinco intentos. |
| `runTasks` | Traducida, `0074ed30`: conserva todas las ramas y llamadas a procesadores aún no traducidos. No se atribuye funcionalidad a esos stubs. La tarea 14 es un no-op contado en la tabla de saltos original. |
| `loadImage` | Traducida, `0074ab70`: archivo/VRA, imagen, máscara blanca y máscara de contorno. Bools recuperados del ensamblado. |
| `loadFont` | Traducida, `0074b440`: fuentes SFML, memoria marcada managed y texturas de tamaños 8/12/16. |
| `uploadTexture` | Traducida, `00751d90`: carga GPU, smooth/repeat y resolución de inserciones duplicadas. |
| `getTexture` | Traducida, `0074cb20`: texturas procesadas `@`, subida diferida y bloqueos durante carga. |
| `getShaderPointer` | Traducida, `00750bb0`: carga diferida de shader Fragment; argumento 2 recuperado en `00750c0a`. |
| `loadShader` | Traducida, `0074c200`: archivo/VRA y uniform `texture`. |
| `processImage` | Traducida, `00750f70`: nueve dibujos por tile, bordes de un píxel y límite incremental de 16 tiles. |
| `getConfig`, `isArchiveLoaded`, `getRoomData`, `getAttackData`, `getPaletteData`, `getBaseRoomDatabase` | Helpers inline reconstruidos de los campos y accesos expandidos. |
| `getLanguageString` | Traducida previamente durante Config. |
| `ResourceArchive::ResourceArchive`, `~ResourceArchive` | Traducidas: inicialización y liberación de bloques y textos. |
| `ResourceArchive::load` | Traducida, `00739180`: zlib, registros contados y tres textos adicionales, en orden rooms/palettes/attacks. |
| `ResourceArchive::getMemoryBlock` | Traducida, `0073aa40`, búsqueda sin inserción. |
| `ResourceArchive::getManagedMemoryBlock`, `isLoaded` | Helpers inline reconstruidos. |
| `ResourceArchive::addResourceLocationDescriptor` | Traducida, `0073ab70`. |
| `ResourceArchive::cleanUpMemoryBlocks` | Traducida, `0073ac30`: conserva managed cuando el argumento es false. |
| `ResourceArchive::inflateFile` | Traducida, `0073ae50`: zlib con buffers de 0x4000 bytes. |
| `ResourceBlock::ResourceBlock`, `~ResourceBlock`, `getSize`, `getData`, `setManaged`, `isManaged` | Traducidas con almacenamiento propietario y getters del contrato. |

## Resto del índice de ResourceManager

Cada nombre restante de `--unit ResourceManager` queda explícitamente fuera del primer bloque:

| Función | Estado / motivo |
|---|---|
| `ImageResource::ImageResource` | Constructor implícito de la cabecera: SFML y flags establecidos por loadImage. No se añade una declaración a la cabecera generada. |
| `Task::~Task` | Destructor implícito de STL; no hay destructor declarado en el contrato. |
| `createNewVra` | Pendiente: creación/exportación del archivo; no necesaria para leer el VRA existente. |
| `getDefaultFont` | Pendiente, siguiente bloque de acceso a recursos. |
| `getGuaranteedUnicodeFont` | Pendiente, siguiente bloque de acceso a recursos. |
| `getIndexOfSound` | Pendiente, audio. |
| `getMusicPointer` | Pendiente, audio. |
| `getTextureAltPalette` | Pendiente, paletas. |
| `getTextureMask` | Pendiente, acceso a máscaras. |
| `getTextureOutlineMask` | Pendiente, acceso a máscaras. |
| `getVFont` | Pendiente, fuente bitmap. |
| `loadClassTextures` | Pendiente, catálogo de clases. |
| `loadFlagIcons` | Pendiente, catálogo de banderas. |
| `loadMusic` | Pendiente, audio. |
| `loadSound` | Pendiente, audio. |
| `loadVFont` | Pendiente, fuente bitmap. |
| `playMusic` | Pendiente, audio. |
| `playSound` (todas las sobrecargas) | Pendiente, audio. |
| `playSoundPriv` | Pendiente, audio. |
| `processPaletteSwap` | Pendiente, paletas. |
| `queueLoadSoundTasks` | Pendiente: catálogo completo de sonido/música. Sigue registrándose como stub. |
| `queueLoadTextureTasks` | Pendiente: catálogo completo de imágenes, clases, banderas y procesados. Sigue registrándose como stub. |
| `reloadAllShaders` | Pendiente, recarga. |
| `reloadAllTexturePaletteSwaps` | Pendiente, recarga de paletas. |
| `setGlobalSfxVolume` | Pendiente, audio. |
| `setMusicChildPercent` | Pendiente, audio. |
| `setMusicVolume` | Pendiente, audio. |
| `setSfxFactor` | Pendiente, audio. |
| `setSpecificMusicVolume` | Pendiente, audio. |
| `stopAllMusic` | Pendiente, audio. cleanUp conserva la llamada y su registro de stub. |
| `updateMusicTransition` | Pendiente, audio. |
| `ResourceArchive::scalar deleting destructor`, `VFont::scalar deleting destructor` | Sin pseudocódigo; los implementa el compilador mediante delete. |
| `defer_func`, destructores `privDefer` | Artefactos de RAII originales; se sustituyen por unique_lock/lock_guard. |
| `snprintf` | CRT: se usa la biblioteca estándar, no se reconstruye MSVC. |

En ResourceArchive siguen pendientes `create`, `createCache`, `loadCache`, `checkCache`,
`writeBlobToStream`, `writeTextureToStream`, `deflateFile` y `zlibError`: no forman parte
de la lectura del archivo existente. Tampoco se han traducido RoomDatabase,
AttackScriptDatabase, PaletteDatabase, EMusic ni ESoundBuffer como parte de esta entrega.

## Constantes y dudas

- Inicializadores `004050f0`…`00405170`: `gfx/`, `sfx/`, `sfx/`, `gfx/font/`, `gfx/shaders/`.
  La ruta de música **también** es `sfx/` en el original.
- Inicializador `00405610`: vector de 18 mutexes.
- FNV-1a32: base `0x811c9dc5`, multiplicador `0x01000193`. Máximo de intentos: 5.
- `ARCHIVE_FORMAT_VERSION=8` ya figuraba recuperada en statics. El lector original lee
  la versión sin rechazar valores distintos; se conserva ese comportamiento.
- El constructor PDB de ResourceArchive tiene `(const char*, bool)`, pero el cuerpo
  expandido solo inicializa campos. Se conservan firma y layout; load usa `data.vra` relativo.
- Endurecimientos limitados a entradas inválidas: detección de truncamiento, carga
  transaccional, liberación al reemplazar un nombre repetido y retorno seguro ante
  bloques ausentes de fuente/shader. No cambian recursos válidos ni introducen RNG.
- cleanUp original deja imágenes CPU, outline textures, cache, roomDatabase y estado
  initialized fuera de su limpieza. Se conserva; las pruebas liberan sus fixtures aparte.
- processImage conserva la columna inicial entre filas al reanudar, como el ensamblado
  `00751322/00751360`. No se corrige silenciosamente esa particularidad del original.
- Los catálogos pendientes implican que initialize actual procesa las dos fuentes y
  cuatro tareas base. No carga automáticamente todos los sprites o sonidos todavía.
  La prueba de textura encola/carga expresamente el recurso probado.

## Verificación ejecutada

- `pwsh tools/build_recon_pc.ps1 -Run`: stubs regenerados, MSYS2/GCC, enlace sin errores,
  ventana SFML nativa y cierre mediante evento normal: **exit code: 0**.
- Esa ejecución usó una copia privada de `data.vra` dentro de `_local/build-recon-pc`.
  Log observado: archivo leído, seis tareas en ocho workers, shader postprocessing
  cargado, inicialización terminada y Cleanup complete. **65** stubs alcanzados;
  ninguno de las nueve funciones solicitadas. ResourceManager aún registra los dos
  catálogos y stopAllMusic.
- `pwsh tools/run_recon_tests.ps1 -Unit ResourceArchive`, con `VAGANTE_VRA` apuntando al
  archivo F: del usuario: **exit 0**. Binario/textos, duplicados, managed, recarga,
  archivos corruptos/truncados/ausentes y lectura del archivo real.
- `pwsh tools/run_recon_tests.ps1 -Unit ResourceManager`, con el mismo `VAGANTE_VRA`:
  **exit 0**. Textura real **172×44**, píxeles/bordes de atlas, try-lock no bloqueante,
  progreso, cancelación, finish incremental y carga síncrona/asíncrona. Fuentes Tiny
  y Noto: glyph solicitado después de liberar bloques no-managed; shader real compilado.
- Generador: reconocimiento corregido de constructores/destructores de clases anidadas
  (Task). Lanzador de pruebas: usa objetos activos de Ninja, excluyendo objetos obsoletos
  de stubs eliminados. Zlib enlazado mediante find_package/ZLIB::ZLIB. Son los cambios
  mínimos de herramientas necesarios para compilar y comprobar este bloque.
- Evidencia local: `_local/resourcemanager-build-run.log`, `_local/resourcearchive-test.log`,
  `_local/resourcemanager-test.log`, `_local/build-recon-pc/{recon.log,logs/log.log}`.
- `data.vra`: 99 768 748 bytes comprimidos, 124 879 064 descomprimidos; 2702 registros
  generales y tres textos adicionales. SHA-256:
  `3ff6d1a4d17ff3c0f7865d328c916887dc0f9030cd60fdcfdcd7e455f7b9958f`.
- Ejecutable PC probado, SHA-256:
  `b4a77ca252377c970ff8b1a04bb6b6a199729a45fd7bf99d0d49d99b6e33a7fe`.

Esto verifica inicialización y recursos en PC. No acredita gameplay ni ejecución en Vita/Vita3K.
