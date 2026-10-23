# ResourceManager, bloque 2 — rama A

Fecha: 2026-10-22. Trabajo local, sin push. Contrato PDB sin modificar.

La carga desde el `data.vra` del usuario completa 2 730 tareas. La pantalla de
carga recibe la textura real y progreso intermedio; no se simula el contador.
Hay 2 395 solicitudes de imagen y 2 393 imágenes únicas, 272 efectos de sonido,
32 músicas, dos fuentes SFML y tres fuentes bitmap. Los 304 registros de audio
incluyen música; no son 304 efectos. Las dos imágenes repetidas conservan la
semántica de actualización del cargador anterior.

## Implementación y evidencia

- `ResourceManager_queues.cpp`: recetas en el orden del ejecutable, incluidos
  cuatro variantes de seis clases, los extras de skeleton/pet, las cuatro
  damsel, 247 banderas, máscaras, atlas, repetición y fuentes bitmap.
  `queueLoadTextureTasks` @0073deb0, `loadFlagIcons` @00744990,
  `queueLoadSoundTasks` @00747260; argumentos de cada llamada contrastados con
  `tools/call_args.py`. ECX selecciona el tipo y XMM2/XMM3 sus atributos.
- `ResourceManager_audio.cpp`: carga de buffers, streaming desde bloques
  administrados, enlaces padre/hijo, puntos de bucle, reproducción, transiciones
  y volúmenes. Se conservan los bytes de música hasta destruir los streams.
- `ResourceManager_sounds.cpp`: ambas sobrecargas de `playSound`, acceso por
  índice, límite de 201 sonidos, supresión de duplicados por frame, limpieza de
  sonidos terminados sin otros propietarios, pitch y volumen/posición.
- `ResourceManager_fonts.cpp`: getters no bloqueantes, carga/reemplazo de VFont
  y parser de marcadores del atlas @007ebd30. El dibujo y la medición de texto
  de VFont siguen en la tarea 6.
- `ResourceManager_images.cpp`: upload de máscaras normal/outline y generación
  de cambios de paleta @00751930 y @004b6f70. Se conserva el alfa, se ignoran
  píxeles transparentes y la imagen fuente se copia antes de modificarla.
- `ResourceManager_tasks.cpp`: se mantiene el despacho del bloque 1. Se añade
  sincronización entre las cargas primarias y sus procesadores dependientes.

### Corrección de sincronización observada

En una repetición anterior a la corrección, la carga no alcanzó 100 %. El log de
la ejecución normal identificó `TASK_IMAGE_REPEAT` para
`monster/last_boss/environment/fog-texture.png`: agotó los cinco intentos antes
de que otro trabajador publicara la imagen. Dequeue no equivale a finalización.

`getNextTask` ahora espera con una variable de condición mientras hay cargas
primarias en curso antes de entregar la cola de procesamiento. No cambia las
recetas, la prioridad entre las dos colas primarias, el límite de reintentos,
las constantes del juego ni introduce llamadas al RNG. Es una adaptación de
sincronización deliberada respecto al ejecutable, sin cambiar layouts PDB.
La prueba reproduce una carga tomada por un trabajador y verifica que otro no
recibe el procesador dependiente hasta que termina la primera.

### Constantes recuperadas y portabilidad

- Pitch: `(float(mt19937()) / 4294967296.0f - 0.5f) * modifier + 1.0f`.
  El helper MSVC @0041c980 consume exactamente una extracción; se conserva su
  conversión a float y no se introduce otra distribución ni otro generador.
- Transiciones: incremento/decremento `0.04f` (`0x3d23d70a`).
- Audio espacial: umbrales 40/300, pan desde 64 con multiplicador
  `0.006666666828095913f`, atenuación inicial 1.5 y volumen limitado a [0,100].
  Los estáticos correspondientes ya estaban recuperados en `statics.cpp`.
- Los volúmenes, modificadores de pitch y puntos de bucle de todas las recetas
  se leen de los inmediatos/constantes usados antes de cada llamada; por ejemplo
  `title_theme.ogg`: 46.084999084472656 segundos.
- El fork SFML original tiene `setPan`; SFML 2.5 estándar no. Se mapea el pan a
  una posición relativa en un círculo unitario, con atenuación OpenAL desactivada
  para aplicar la fórmula del juego. La paridad auditiva con el fork original,
  especialmente para fuentes estéreo, no está demostrada.
- SFML 2.5 hace el bucle mediante `Music::onLoop`, sin llamar a `onSeek` como
  esperaba el original. `setLoopPoint` configura también `setLoopPoints` de
  SFML y se sincroniza el contador al alcanzar EOF. La cuantización temporal
  de SFML puede diferir en la precisión de muestras respecto al fork original.
- No se añaden APIs Windows, DirectInput, XInput ni Steam. Se omite el guard del
  hilo de servidor en `playSoundPriv` porque la red está excluida.

## Verificación ejecutada

1. `pwsh tools/build_recon_pc.ps1 -Run`: regeneró stubs, compiló/enlazó con
   MSYS2, abrió `vagante.exe` y cerró normalmente mediante su ventana.
   **exit code: 0**. `recon.log`: **57** funciones pendientes alcanzadas.
2. `pwsh tools/run_recon_tests.ps1 -Unit ResourceManagerLoading`, con
   `VAGANTE_TEST_ARCHIVE` apuntando a la copia privada del archivo real: pasó.
   Repetición directa del mismo ejecutable en una carpeta de ejecución nueva:
   pasó. Ambas completaron **2730/2730**, sin `requeueTask Failed`.
3. La prueba verificó imágenes y audio decodificados, duraciones, tres atlas de
   fuentes bitmap, glyph nuevo tras liberar bloques no administrados, música
   enlazada y sus ganancias 30/10, punto de bucle, pitch y consumo del RNG,
   duplicados por frame, volúmenes posteriores, máscaras y atlas con bordes.
   Incluye rechazo de registros ausentes y paletas incompatibles.
4. `pwsh tools/run_recon_tests.ps1 -Unit ResourceManager`: pasó. El pequeño
   fixture del bloque 1 usa ahora el modo de descriptores para probar ambas
   modalidades de scheduling; la carga completa corresponde al test nuevo.

Evidencia local, excluida de Git:

- `_local/rm7b-final-build-run.log`, `_local/build-recon-pc.log` y
  `_local/build-recon-pc/recon.log`.
- `_local/rm7b-final-tests.log`, `_local/rm7b-repeat-test.log` y
  `_local/rm7b-regression-tests.log`.
- `_local/recon-tests/ResourceManagerLoading/work/run-2064474929100/progress.csv`:
  82/2730 (3.00 %), 431/2730 (15.79 %), 1024/2730 (37.51 %) y 2730/2730.
  Captura SFML `loading-1.png` en la misma carpeta, inspeccionada visualmente.
- Archivo real: SHA-256
  `3ff6d1a4d17ff3c0f7865d328c916887dc0f9030cd60fdcfdcd7e455f7b9958f`.

## Límites y dudas

La carga y el render de su indicador están comprobados en PC. No se genera un
VPK ni se valida gameplay en Vita/Vita3K en este bloque. No se incorporan datos,
imágenes, audio ni pseudocódigo comerciales a Git.

La cabecera no declara `getSound` ni `getFont`: se implementan las APIs reales
`playSound`/índice y `getDefaultFont`/`getGuaranteedUnicodeFont`/`getVFont`.
No se inventan firmas ni se modifican cabeceras regeneradas.

`RoomDatabase`, `AttackScriptDatabase::initialize`, `PaletteDatabase::initialize`
y `getPaletteGroupList` siguen siendo stubs de otras unidades. Por ello se
comprueba el procesador de paletas con fixtures; todavía no se cargan los grupos
de paletas del JSON original. `Client`/`GameEngine`, Sprite y el resto de VFont
también tienen trabajo pendiente. El éxito de carga no equivale a juego completo.

Se conservan las omisiones de `cleanUp` documentadas en el bloque 1, entre ellas
la retención de imágenes CPU y el estado `initialized`. No se amplía aquí su
semántica. Los guards y devoluciones false para bloques ausentes evitan los
null dereference del original sobre datos incompletos.

## Inventario por función

El inventario siguiente incluye la unidad completa para distinguir el bloque 2
de lo ya traducido y de las operaciones ajenas a esta entrega.

| Funcion del indice | Estado |
|---|---|
| `ResourceArchive::`scalar deleting destructor'` | Sin pseudocodigo; helper/destructor generado por el compilador, no se reconstruye. |
| `ResourceManager::ImageResource::ImageResource` | Constructor de copia implicito de sf::Image y flags; no se redefine. |
| `ResourceManager::Task::Task` | Traducida en bloque 1; conservada y ejercitada por la carga completa. |
| `ResourceManager::Task::~Task` | Destructor implicito de STL, generado por el compilador; no se redefine. |
| `ResourceManager::cancelInitialization` | Traducida en bloque 1; conservada y ejercitada por la carga completa. |
| `ResourceManager::cleanUp` | Traducida en bloque 1; conservada y ejercitada por la carga completa. |
| `ResourceManager::createNewVra` | Fuera del bloque: escritura de un archivo nuevo; esta entrega lee el archivo externo. |
| `ResourceManager::finishInitialization` | Traducida en bloque 1; conservada y ejercitada por la carga completa. |
| `ResourceManager::getDefaultFont` | Traducida en bloque 2: `ResourceManager_fonts.cpp`. |
| `ResourceManager::getGuaranteedUnicodeFont` | Traducida en bloque 2: `ResourceManager_fonts.cpp`. |
| `ResourceManager::getIndexOfSound` | Traducida en bloque 2: `ResourceManager_sounds.cpp`. |
| `ResourceManager::getLanguageString` | Traducida en bloque 1; conservada y ejercitada por la carga completa. |
| `ResourceManager::getMusicPointer` | Traducida en bloque 2: `ResourceManager_audio.cpp`. |
| `ResourceManager::getNewLoadingTexture` | Traducida en bloque 1; conservada y ejercitada por la carga completa. |
| `ResourceManager::getNextTask` | Traducida en bloque 1, ejercitada de nuevo; `getNextTask` recibe la correccion de sincronizacion de bloque 2. |
| `ResourceManager::getShaderPointer` | Traducida en bloque 1; conservada y ejercitada por la carga completa. |
| `ResourceManager::getTexture` | Traducida en bloque 1; conservada y ejercitada por la carga completa. |
| `ResourceManager::getTextureAltPalette` | Fuera del bloque: lookup/upload de variantes por grupo; el parser JSON de PaletteDatabase sigue pendiente. |
| `ResourceManager::getTextureMask` | Traducida en bloque 2: `ResourceManager_images.cpp`. |
| `ResourceManager::getTextureOutlineMask` | Traducida en bloque 2: `ResourceManager_images.cpp`. |
| `ResourceManager::getVFont` | Traducida en bloque 2: `ResourceManager_fonts.cpp`. |
| `ResourceManager::initThreadsAreDone` | Traducida en bloque 1; conservada y ejercitada por la carga completa. |
| `ResourceManager::initialize` | Traducida en bloque 1; conservada y ejercitada por la carga completa. |
| `ResourceManager::loadClassTextures` | Traducida en bloque 2: `ResourceManager_queues.cpp`. |
| `ResourceManager::loadFlagIcons` | Traducida en bloque 2: `ResourceManager_queues.cpp`. |
| `ResourceManager::loadFont` | Traducida en bloque 1; conservada y ejercitada por la carga completa. |
| `ResourceManager::loadImage` | Traducida en bloque 1; conservada y ejercitada por la carga completa. |
| `ResourceManager::loadMusic` | Traducida en bloque 2: `ResourceManager_audio.cpp`. |
| `ResourceManager::loadShader` | Traducida en bloque 1; conservada y ejercitada por la carga completa. |
| `ResourceManager::loadSound` | Traducida en bloque 2: `ResourceManager_audio.cpp`. |
| `ResourceManager::loadVFont` | Traducida en bloque 2: `ResourceManager_fonts.cpp`. |
| `ResourceManager::playMusic` | Traducida en bloque 2: `ResourceManager_audio.cpp`. |
| `ResourceManager::playSound` | Traducida en bloque 2: `ResourceManager_sounds.cpp`. |
| `ResourceManager::playSoundPriv` | Traducida en bloque 2: `ResourceManager_sounds.cpp`. |
| `ResourceManager::processImage` | Traducida en bloque 1; conservada y ejercitada por la carga completa. |
| `ResourceManager::processPaletteSwap` | Traducida en bloque 2: `ResourceManager_images.cpp`. |
| `ResourceManager::queueLoadSoundTasks` | Traducida en bloque 2: `ResourceManager_queues.cpp`. |
| `ResourceManager::queueLoadTextureTasks` | Traducida en bloque 2: `ResourceManager_queues.cpp`. |
| `ResourceManager::queueTask` | Traducida en bloque 1; conservada y ejercitada por la carga completa. |
| `ResourceManager::reloadAllShaders` | Fuera del bloque: recarga de shaders de desarrollo. |
| `ResourceManager::reloadAllTexturePaletteSwaps` | Fuera del bloque: recarga de variantes de desarrollo. |
| `ResourceManager::requeueTask` | Traducida en bloque 1; conservada y ejercitada por la carga completa. |
| `ResourceManager::runTasks` | Traducida en bloque 1; conservada y ejercitada por la carga completa. |
| `ResourceManager::setGlobalSfxVolume` | Traducida en bloque 2: `ResourceManager_audio.cpp`. |
| `ResourceManager::setMusicChildPercent` | Traducida en bloque 2: `ResourceManager_audio.cpp`. |
| `ResourceManager::setMusicVolume` | Traducida en bloque 2: `ResourceManager_audio.cpp`. |
| `ResourceManager::setSfxFactor` | Traducida en bloque 2: `ResourceManager_audio.cpp`. |
| `ResourceManager::setSpecificMusicVolume` | Traducida en bloque 2: `ResourceManager_audio.cpp`. |
| `ResourceManager::stopAllMusic` | Traducida en bloque 2: `ResourceManager_audio.cpp`. |
| `ResourceManager::updateMusicTransition` | Traducida en bloque 2: `ResourceManager_audio.cpp`. |
| `ResourceManager::uploadTexture` | Traducida en bloque 1; conservada y ejercitada por la carga completa. |
| `VFont::`scalar deleting destructor'` | Sin pseudocodigo; helper/destructor generado por el compilador, no se reconstruye. |
| `defer_func<<lambda_56473b528101c599228ef448cd863f2f> >` | Sin pseudocodigo; helper/destructor generado por el compilador, no se reconstruye. |
| `privDefer<<lambda_059b111fdba9a681a9ce321b4cb6b2d6> >::~privDefer<<lambda_059b111fdba9a681a9ce321b4cb6b2d6> >` | Sin pseudocodigo; helper/destructor generado por el compilador, no se reconstruye. |
| `privDefer<<lambda_0dc3ccfc8deadb43ee274e4f2c533ee3> >::~privDefer<<lambda_0dc3ccfc8deadb43ee274e4f2c533ee3> >` | Sin pseudocodigo; helper/destructor generado por el compilador, no se reconstruye. |
| `privDefer<<lambda_18d5ce71cab974002c2307556ef0d746> >::~privDefer<<lambda_18d5ce71cab974002c2307556ef0d746> >` | Sin pseudocodigo; helper/destructor generado por el compilador, no se reconstruye. |
| `privDefer<<lambda_199397be6a710b2816c1bcad0c17b1f8> >::~privDefer<<lambda_199397be6a710b2816c1bcad0c17b1f8> >` | Sin pseudocodigo; helper/destructor generado por el compilador, no se reconstruye. |
| `privDefer<<lambda_1dc2b18d91f239e4dd67b7e7071bb747> >::~privDefer<<lambda_1dc2b18d91f239e4dd67b7e7071bb747> >` | Sin pseudocodigo; helper/destructor generado por el compilador, no se reconstruye. |
| `privDefer<<lambda_225ffe4e9000e0ae097536fb1a1c2fb0> >::~privDefer<<lambda_225ffe4e9000e0ae097536fb1a1c2fb0> >` | Sin pseudocodigo; helper/destructor generado por el compilador, no se reconstruye. |
| `privDefer<<lambda_56473b528101c599228ef448cd863f2f> >::~privDefer<<lambda_56473b528101c599228ef448cd863f2f> >` | Sin pseudocodigo; helper/destructor generado por el compilador, no se reconstruye. |
| `privDefer<<lambda_5b5b97329cbeb44b0460232ede357fb0> >::~privDefer<<lambda_5b5b97329cbeb44b0460232ede357fb0> >` | Sin pseudocodigo; helper/destructor generado por el compilador, no se reconstruye. |
| `privDefer<<lambda_5ef84ff0969f411d5dcab7e07834e06e> >::~privDefer<<lambda_5ef84ff0969f411d5dcab7e07834e06e> >` | Sin pseudocodigo; helper/destructor generado por el compilador, no se reconstruye. |
| `privDefer<<lambda_693867a88c85f58b123c10e3032a3efb> >::~privDefer<<lambda_693867a88c85f58b123c10e3032a3efb> >` | Sin pseudocodigo; helper/destructor generado por el compilador, no se reconstruye. |
| `privDefer<<lambda_6e09560ee988ff12f5ae36fc9a839211> >::~privDefer<<lambda_6e09560ee988ff12f5ae36fc9a839211> >` | Sin pseudocodigo; helper/destructor generado por el compilador, no se reconstruye. |
| `privDefer<<lambda_7e63b3928acb1440644ded45d3e2de41> >::~privDefer<<lambda_7e63b3928acb1440644ded45d3e2de41> >` | Sin pseudocodigo; helper/destructor generado por el compilador, no se reconstruye. |
| `privDefer<<lambda_8e75537f8d88b5dbabd5553eb8109d91> >::~privDefer<<lambda_8e75537f8d88b5dbabd5553eb8109d91> >` | Sin pseudocodigo; helper/destructor generado por el compilador, no se reconstruye. |
| `privDefer<<lambda_8f8c5ae1e8eb041e6bd3b01fe860689b> >::~privDefer<<lambda_8f8c5ae1e8eb041e6bd3b01fe860689b> >` | Sin pseudocodigo; helper/destructor generado por el compilador, no se reconstruye. |
| `privDefer<<lambda_a8e74398ee3a471eb70b5fcfdae80b88> >::~privDefer<<lambda_a8e74398ee3a471eb70b5fcfdae80b88> >` | Sin pseudocodigo; helper/destructor generado por el compilador, no se reconstruye. |
| `privDefer<<lambda_ae345c3a2d230acfef4672d541885cc8> >::~privDefer<<lambda_ae345c3a2d230acfef4672d541885cc8> >` | Sin pseudocodigo; helper/destructor generado por el compilador, no se reconstruye. |
| `privDefer<<lambda_e02e8a9cf461022ff630756c67e6405d> >::~privDefer<<lambda_e02e8a9cf461022ff630756c67e6405d> >` | Sin pseudocodigo; helper/destructor generado por el compilador, no se reconstruye. |
| `privDefer<<lambda_e5e6aa4f27f045059dc3ea029c6574cc> >::~privDefer<<lambda_e5e6aa4f27f045059dc3ea029c6574cc> >` | Sin pseudocodigo; helper/destructor generado por el compilador, no se reconstruye. |
| `privDefer<<lambda_e8b346652bcaa3bdf78a64482a19a37b> >::~privDefer<<lambda_e8b346652bcaa3bdf78a64482a19a37b> >` | Sin pseudocodigo; helper/destructor generado por el compilador, no se reconstruye. |
| `snprintf` | Funcion de CRT; se usa la biblioteca estandar del compilador. |

Ambas sobrecargas de `ResourceManager::playSound` (nombre+loop y nombre+x+y+loop) estan traducidas juntas. Lo mismo ocurre con `ESound::setGamePosition`.

### Dependencias y metodos inline necesarios

| Funcion | Estado |
|---|---|
| `ESoundBuffer::ESoundBuffer` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_audio.cpp`. |
| `ESoundBuffer::~ESoundBuffer` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_audio.cpp`. |
| `ESoundBuffer::setDefaultAttenuation` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_audio.cpp`. |
| `EMusic::EMusic` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_audio.cpp`. |
| `EMusic::~EMusic` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_audio.cpp`. |
| `EMusic::timeToSample` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_audio.cpp`. |
| `EMusic::sampleToTime` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_audio.cpp`. |
| `EMusic::openFromFile` | Traducida desde pseudocodigo en `ResourceManager_audio.cpp`. |
| `EMusic::openFromMemory` | Traducida desde pseudocodigo en `ResourceManager_audio.cpp`. |
| `EMusic::setLoopPoint` | Traducida desde pseudocodigo en `ResourceManager_audio.cpp`. |
| `EMusic::onGetData` | Traducida desde pseudocodigo en `ResourceManager_audio.cpp`. |
| `EMusic::onSeek` | Traducida desde pseudocodigo en `ResourceManager_audio.cpp`. |
| `EMusic::play` | Traducida desde pseudocodigo en `ResourceManager_audio.cpp`. |
| `EMusic::stop` | Traducida desde pseudocodigo en `ResourceManager_audio.cpp`. |
| `EMusic::pause` | Traducida desde pseudocodigo en `ResourceManager_audio.cpp`. |
| `EMusic::hasChild` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_audio.cpp`. |
| `EMusic::hasParent` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_audio.cpp`. |
| `EMusic::getChildPercent` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_audio.cpp`. |
| `EMusic::setChild` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_audio.cpp`. |
| `EMusic::getVolume` | Traducida desde pseudocodigo en `ResourceManager_audio.cpp`. |
| `EMusic::setChildPercent` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_audio.cpp`. |
| `EMusic::setVolume` | Traducida desde pseudocodigo en `ResourceManager_audio.cpp`. |
| `ResourceManager::setMusicLoopPoint` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_audio.cpp`. |
| `ResourceManager::getMusicMap` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_audio.cpp`. |
| `VFont::CharacterInfo::CharacterInfo` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_fonts.cpp`. |
| `VFont::loadFromImage` | Traducida desde pseudocodigo en `ResourceManager_fonts.cpp`. |
| `PaletteDatabase::Palette::generatePaletteSwap` | Traducida desde pseudocodigo en `ResourceManager_images.cpp`. |
| `ESound::ESound` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_sounds.cpp`. |
| `ESound::~ESound` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_sounds.cpp`. |
| `ESound::setVolume` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_sounds.cpp`. |
| `ESound::getVolume` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_sounds.cpp`. |
| `ESound::setSpecificVolume` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_sounds.cpp`. |
| `ESound::getSpecificVolume` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_sounds.cpp`. |
| `ESound::setAttenuation` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_sounds.cpp`. |
| `ESound::isGlobal` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_sounds.cpp`. |
| `ESound::setGamePosition` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_sounds.cpp`. |
| `ESound::getGamePosition` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_sounds.cpp`. |
| `ESound::debugGetSoundName` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_sounds.cpp`. |
| `ESound::updatePanAndVolume` | Traducida desde pseudocodigo en `ResourceManager_sounds.cpp`. |
| `ResourceManager::debugGetPlayingSounds` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_sounds.cpp`. |
| `ResourceManager::getSoundNameByIndex` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_sounds.cpp`. |
| `ResourceManager::playSoundIndex` | Traducida; reconstruida desde el contrato y los accesos inline de los llamadores en `ResourceManager_sounds.cpp`. |
