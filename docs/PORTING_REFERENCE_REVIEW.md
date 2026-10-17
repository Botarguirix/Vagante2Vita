# Evaluación de referencias para el port de Vagante

Fecha de revisión: 2026-10-16.

## Aplicación verificada en la iteración 082

Revisé nuevamente las dos referencias para decidir si podían aportar una ruta
de recompilación o extracción directa. `repentogxm` genera C desde una versión
concreta del ejecutable de Isaac (el README fija tamaño y SHA-256), junto con
su runtime y reemplazos nativos; no es un recompilador PE genérico. D2Vita
ejecuta el PE original mediante su loader Win32, puente de imports, dynarec
x86→ARMv7 y shims específicos, y sus utilidades de archivos se enfocan en MPQ
de Diablo II. Adoptar cualquiera exigiría portar su infraestructura y adaptar
las dependencias/APIs de Vagante; no recompilaría el juego con solo cambiar la
ruta del EXE.

La parte transferible ya pasó una comprobación sobre los datos locales: el
auditor comparó el bundle privado de ACT_ONE 082 con `_local/assets` y encontró
74/74 recursos originales con el mismo tamaño y SHA-256, cero ausencias o
diferencias. Las tres imágenes de terreno generadas se identifican como
generadas, no como recursos extraídos. El manifiesto completo del bundle
valida 100 archivos. El archivo fuente `data.vra` no está en el workspace, así
que no se puede repetir aquí la extracción desde el contenedor ni vincular el
inventario previo a una copia actual mediante hash. El resultado está en
`_local/analysis/repentogxm-d2vita-iteration082-audit.json` (solo nombres,
tamaños y hashes; no copia recursos al repositorio).

La decisión práctica es conservar la reconstrucción C nativa y el puente de
recursos externo inspirado por ambas referencias. El VPK 00.82 que ya se
compiló es el runtime reconstruido, no una recompilación estática del EXE
comercial. El procedimiento reproducible para extraer desde una copia propia
cuando `data.vra` esté disponible sigue siendo el de la sección de iteración
080; sus archivos extraídos permanecen locales y el paquete solo recibe los
recursos seleccionados.

## Aplicación en la iteración 080: verificación del puente de recursos

La arquitectura de D2Vita separa sus utilidades host para leer MPQ del runtime
que carga datos aportados por el usuario; Repentogxm también mantiene los
recursos del juego fuera del VPK. Para Vagante, esta separación ya existe con
el extractor propio de `.vra` y los paquetes externos VWI/VWE/VLT. No conviene
conectar el extractor MPQ de Diablo II ni distribuir los archivos extraídos.

Verifiqué localmente `_local/delivery-act-one-campaign077` contra su
`manifest.json`: los 100 archivos y 2,843,768 bytes pasan tamaño y SHA-256. De
las 78 imágenes que el manifiesto atribuye a recursos originales, las 78
coinciden byte por byte con su ruta correspondiente bajo `_local/assets`.
Esto demuestra que la transición extracción → paquete ACT_ONE conserva los
recursos seleccionados sin incluir la extracción completa. El VPK 00.80 y sus
límites de prueba están documentados en [ITERATION80.md](ITERATION80.md).

El inventario de extracción disponible registra `data.vra` con SHA-256
`3ff6d1a4d17ff3c0f7865d328c916887dc0f9030cd60fdcfdcd7e455f7b9958f`, 2,705
registros y 124,879,064 bytes descomprimidos. El contenedor original no está
presente en `_local`, así que este turno no pudo repetir la extracción ni
confirmar que el inventario corresponda a una instalación actual. Cuando esté
disponible una copia legítima del juego, la ruta reproducible es:

```powershell
python tools/vra.py '<ruta-a-data.vra>' --out _local/analysis/archive.json --extract _local/assets-from-data-vra
python tools/prepare_world_bundle.py --validate _local/delivery-act-one-campaign077
python tools/audit_asset_bundle.py _local/assets-from-data-vra _local/delivery-act-one-campaign077 --source-archive '<ruta-a-data.vra>'
```

El directorio de extracción debe estar vacío antes de ejecutar `vra.py`.

Para regenerar el bundle, se vuelven a ejecutar `tools/export_act_one_world.py`
y `tools/prepare_world_bundle.py` con los paquetes del mundo y assets locales
actuales; el VPK solo consume el bundle preparado en `ux0:data`, no el
contenedor `.vra`. Por separado, la recompilación estática de Repentogxm exige
un traductor específico para su ejecutable PE, y D2Vita exige su runtime x86,
bridge de imports y shims. Ningún proyecto aporta un recompilador genérico
para el PE de Vagante, por lo que la reconstrucción nativa con reglas
verificadas en Ghidra sigue siendo la ruta viable para completar ACT_ONE.

## Aplicación en la iteración 076

La guía de `repentogxm` confirma que su recompilador estático depende de un PE
exacto, identificado por tamaño y SHA-256, además de un runtime propio; no es un
traductor x86 genérico que se pueda aplicar al ejecutable de Vagante. D2Vita
confirma otra arquitectura: loader Win32/x86, dynarec derivado de Box86,
bridges de imports y shims del sistema, con los MPQ del usuario en
`ux0:data/d2vita/`. Su propia documentación distingue herramientas host para
inspeccionar recursos del build de Vita y mantiene el EXE/datos comerciales
fuera del repositorio. Para Vagante adoptamos la parte útil de ese flujo sin
reutilizar su engine: recursos de la copia local → paquetes VWI/VWE/VLT
versionados y con hashes → runtime nativo Vita que carga datos externos.

La integración práctica queda en `tools/audit_asset_bundle.py`: valida el
manifiesto del bundle y compara hashes contra rutas conocidas de la extracción
local, sin duplicar archivos. El bundle privado 076 registra 100 archivos y
mantiene el contenido comercial fuera del VPK. No se encontró `data.vra` en
las carpetas `_local/assets`, `_local/analysis` ni en las entregas, por lo que
la correspondencia con el archivo de origen aún no se puede verificar en esta
sesión; el inventario previo de 2,705 archivos no prueba por sí solo qué copia
instalada lo produjo. La herramienta acepta `--source-archive` cuando la copia
original vuelva a estar disponible y solo registra tamaño/hash, nunca copia
el archivo.

La recompliación directa de Vagante sigue siendo una investigación de alto
riesgo: el EXE PE32 x86 no resuelve sus dependencias SFML/OpenGL/OpenAL/
Steamworks ni aporta las adaptaciones de sistema que estos proyectos
construyeron expresamente para Isaac y Diablo II. El siguiente avance hacia el
mundo completo es cerrar la evidencia de los pisos 1–2, las entidades/loot y
la transición en el runtime reconstruido, manteniendo los datos propios del
usuario fuera del repositorio y VPK.

## Aplicación en la iteración 074

Las referencias son guías de arquitectura y validación, no código
intercambiable: Repentogxm traduce un PE y runtime concretos, mientras D2Vita
separa un motor Win32/x86 genérico de los shims del juego. Para Vagante se
mantiene esa separación en sus propios formatos: `tools/vra.py` lee `.vra`, el
preparador produce el bundle externo y el runtime Vita consume tablas y
recursos versionados sin copiar el archivo fuente al VPK. Se aplicó además la
disciplina de verificar cada regla contra el binario local: Bat ahora usa su
daño de contacto observado (1–6) y el bloqueo de 30 frames de Ghidra. El daño
usa un flujo local reproducible porque aún no se ha conectado el MT global del
juego; por tanto, no ofrece paridad de seed.

La siguiente frontera relevante es la IA y el ataque de Goblin: su daño no
debe modelarse como contacto plano porque el original lo deriva de un arma.
Los recursos comerciales continúan fuera del repositorio y del VPK.

## Aplicación en la iteración 073 (2026-10-16)

Las dos referencias confirman que conviene separar código propio y datos
comerciales. Esa separación ya está operativa: el runtime 073 se compila a VPK
y el ACT_ONE privado se entrega como bundle externo verificado (94 archivos,
SHA-256 por recurso). La extracción local de Vagante aporta el loot VLT1, sus
atlas y sprites originales de los enemigos. Ghidra sostiene las reglas del
Boulder y los constructores/cajas de los enemigos. El bundle manual 072 omitía
loot obligatorio; la generación reproducible 073 corrige esa inconsistencia.
No se copiaron extractores MPQ ni runtimes ajenos.

La recompilación directa sigue siendo una línea de investigación distinta. A
diferencia de los traductores específicos de Repentogxm y del runtime Win32/x86
de D2Vita, Vagante requiere resolver su propio conjunto de imports,
dependencias SFML/OpenGL/OpenAL/Steamworks y el ABI del binario. Aunque el EXE
local es PE32 x86, esa coincidencia de arquitectura no aporta los bridges ni
las adaptaciones de sistema. Traducir el pseudocódigo de Ghidra tampoco produce
código fuente compilable. La ruta probada que da avance al mundo sigue siendo
reconstruir las funciones con evidencia de Ghidra/PDB y reutilizar recursos
locales mediante el bundle no invasivo.

El VPK 073 compila y la suite Python pasa 140 pruebas. Su smoke aislado de
Vita3K preparó una replay de salida ACT_ONE, pero agotó 45 segundos sin iniciar
el emulador ni crear logs; no hay prueba de carga del VPK ni de gameplay. Véase
[ITERATION73.md](ITERATION73.md).

## Revalidación práctica (2026-10-16)

En una revisión posterior se volvió a ejecutar el VPK `00.53` contra el bundle
privado de 67 archivos, con la misma copia aislada de Vita3K `0.2.1.4170`.
SDL/Vulkan y Qt/OpenGL agotaron 90 segundos; ambos procesos devolvieron `-1` y
no dejaron log del emulador ni sesión nativa del juego. Las replays general y
de salida se prepararon, pero ninguna llegó al runtime. El VPK conserva el hash
`CDF3A56CEF30E54A5110581F07D3B692DCB4377FABE22937E806F535B46C0C3E`. Se
compilaron los módulos runtime afectados con ARM y warnings estrictos y la
suite Python pasó 119/119. El resultado no cambia la limitación: Vita3K aún no
aporta evidencia sobre el arranque o el gameplay.

Se reconstruyó el VPK 00.51 desde las fuentes actuales con VitaSDK de Windows,
en `_local/build-reference-recheck-051/`. El paquete abre como ZIP válido,
contiene las siete entradas esperadas y sus siete contenidos son idénticos byte
por byte a los del VPK documentado en `docs/ITERATION51.md`. El hash del VPK
completo cambia porque el empaquetador escribe la hora actual en los registros
ZIP de `param.sfo` y `eboot.bin`; los hashes de los archivos internos sí
coinciden. Esto comprueba la compilación y el contenido, pero no es una prueba
de arranque ni de gameplay en Vita3K. No se repitió Vita3K porque el fallo
anterior ocurre antes del juego y esta reconstrucción contiene exactamente los
mismos binarios.

El bundle privado `_local/act-one-campaign050` también valida con manifiesto,
versión y hashes: 69 archivos. La suite completa pasa (119 pruebas). La
extracción original de `data.vra` sigue registrada como 2,705 entradas; no hizo
falta duplicar más de 124 MB de recursos para esta comprobación. WSL sigue sin
iniciar en este equipo (`Wsl/Service/CreateInstance/E_ACCESSDENIED`), por lo
que esta compilación verificable se hizo con el script VitaSDK nativo de
Windows. La ruta de WSL queda pendiente de recuperar si se requiere una
reproducción específicamente dentro de Linux.

VPK reconstruido: `_local/build-reference-recheck-051/Vagante2Vita-iteration051.vpk`.
SHA-256 del contenedor ZIP: `82822C16EDA0E623EBD38909F770B49C273E913375BF9AACED3029EB53E201E4`.
Los siete hashes internos coinciden con el VPK 00.51 documentado; véase el
hash de referencia allí para comparar el contenedor con sus marcas de tiempo.

## Qué aportan los dos proyectos

`repentogxm` recompila estáticamente un ejecutable x86 concreto y genera C
para compilarlo junto con un runtime Vita. El repositorio es experimental y
específico para Isaac: el traductor reconoce el EXE y su hash, y las llamadas
de plataforma se resuelven con su runtime. Su lección aplicable es evaluar una
traducción binaria solo con un ejecutable de referencia identificado y pruebas
de equivalencia por función; no es un traductor listo para cualquier juego.

`D2Vita` tomó otro camino: carga el ejecutable Windows x86 original mediante
un runtime Win32, un puente de imports y un dynarec x86→ARMv7; sustituye la
capa de sistema por adaptadores Vita y conserva los recursos comerciales
fuera del VPK. Su arquitectura genérica vive aparte de los hooks específicos
de Diablo. El repositorio documenta también inspección/extracción MPQ, formato
propio de Diablo II.

La parte útil no es copiar ninguno de esos ports literalmente, sino distinguir
las tres rutas y sus costes:

| Ruta | Qué se reutiliza | Encaje para Vagante ahora |
|---|---|---|
| Recompilación estática (Repentogxm) | Identificar y fijar el binario de referencia; generar traducción reproducible; conservar los datos del usuario fuera del VPK; probar diferencias contra el original. | Investigación futura. El traductor de Repentogxm es específico de Isaac y su PE/hashes; no traduce Vagante por sí solo. Hace falta un traductor x86 propio y resolver imports/runtime. |
| Runtime x86 (D2Vita) | Separar runtime genérico, adaptadores Vita, hooks del juego y herramientas host; mantener EXE/datos originales locales. | Alternativa de alto riesgo. Vagante depende de SFML/OpenGL/Steamworks y el runtime de D2Vita ejecuta Diablo II con su conjunto de APIs. No se puede asumir que ese runtime acepte Vagante. |
| Reconstrucción nativa actual | Extraer/analizar datos en host, convertir solo formatos requeridos a paquetes versionados, leerlos desde el runtime Vita y validar la jugabilidad con evidencia de Ghidra/PDB. | Es la ruta que ya está conectada al primer mundo y evita añadir un Win32 completo a la Vita. |

## Qué ya se aprovechó y qué se comprobó

- **Recursos:** `tools/vra.py` decodifica el contenedor `.vra` de Vagante, valida
  límites y rutas, informa entradas duplicadas y hashes, y escribe la extracción
  en `_local/assets`. El inventario local (`_local/analysis/archive.json`)
  registra el SHA-256 del archivo inspeccionado, 2,705 archivos y 124,879,064
  bytes descomprimidos. En esta sesión no se halló el contenedor de origen en
  las rutas habituales, por lo que no se pudo verificar que ese inventario
  corresponda a la copia instalada actualmente. Los recursos extraídos siguen
  disponibles localmente para continuar el mundo.
- **Capa de transición:** `tools/prepare_world_bundle.py` ya cumple el patrón no
  invasivo de D2Vita/Repentogxm: convierte recursos locales seleccionados a
  paquetes versionados, genera un manifiesto SHA-256 y valida rutas, tamaños y
  hashes. Los paquetes de prueba viven en `_local/act-one-campaign*/` y el
  runtime puede consumirlos desde los datos externos. El VPK contiene el
  runtime, no el archivo comercial completo. Desde esta revisión, el manifiesto
  también puede registrar el tamaño y SHA-256 del `data.vra` fuente con
  `--source-archive`, sin copiar el archivo. Esta opción queda pendiente de
  probarse con el archivo fuente disponible; el inventario anterior conserva
  su propio hash, pero el paquete 050 no registra dicha relación.
- **Recompilación:** no se intentó ejecutar el traductor de Repentogxm ni el
  runtime winx86 de D2Vita como si fueran herramientas genéricas. El primero
  genera código para Isaac; el segundo depende de su propio loader, bridges y
  shims. Reutilizarlos sin verificar ABI e imports daría una falsa impresión
  de avance. Ghidra sí aporta evidencia de comportamiento, pero su pseudocódigo
  no equivale a C/C++ decompilado que pueda compilarse tal cual.
- **Build disponible:** el artefacto vigente es
  `_local/build-act-one-073/Vagante2Vita-iteration073.vpk` (título
  `VAGV00006`, versión `00.73`; SHA-256
  `68DC7533312DBC6A27445E3311EB257B14CAF24F809150149FC306C9FC3752AD`). Es
  una compilación del runtime nativo, no un port recompilado del EXE comercial.
  La ejecución aislada de Vita3K 073 terminó antes de iniciar el emulador; el
  VPK no está validado en gameplay.

## Límite de extracción y distribución

El extractor sirve para inspeccionar y preparar la copia del usuario, no para
publicar sprites, música, fuentes o el ejecutable original. Las herramientas,
fixtures sintéticos y código de runtime pueden ir en Git; los recursos
comerciales y sus pseudocódigos permanecen en `_local`. El extractor de MPQ de
D2Vita no se debe conectar a `.vra`: el límite limpio es un backend extractor
por formato que emite los recursos locales que requiere el paquete Vagante.

Fuentes primarias:

- [repentogxm README](https://github.com/0xl0cal/repentogxm)
- [D2Vita arquitectura](https://github.com/Franckrst/D2Vita/blob/main/ARCHITECTURE.md)
- [D2Vita README](https://github.com/Franckrst/D2Vita)
- [sitio de D2Vita](https://franckrst.github.io/D2Vita/)
- [D2Vita roadmap y estado verificado](https://github.com/Franckrst/D2Vita/blob/main/ROADMAP.md)
- [Repentogxm plan](https://github.com/0xl0cal/repentogxm/blob/main/PLAN.md)

## Evidencia de Vagante en este workspace

La copia de referencia identificada en `_local/analysis/binary.json` es un PE32
x86 (`machine=0x14c`), de 8,322,560 bytes, con base `0x400000`; importa
`OPENGL32.dll`. Su hash es
`9af1d8cc204aca2be94f597af21ced50d77d09c1ff8f998def5f2920c3c4624c`.
También hay un PDB local de 64,974,848 bytes. La base Ghidra y la extracción
permanecen en `_local`, fuera de Git.

El juego no usa MPQ: `tools/vra.py` inspecciona el archivo `.vra` propio,
valida el flujo zlib y las rutas antes de extraer. El inventario local registra
2,705 entradas (2,702 declaradas en cabecera), 124,879,064 bytes descomprimidos
y 2,705 archivos extraídos en `_local/assets`. Incluye PNG, OGG, WAV, fuentes,
shaders y JSON. El delta de tres entradas está registrado en el informe y no se
debe ocultar al empaquetar. Los datos y sprites comerciales siguen siendo
privados y no se incorporan al VPK ni al repositorio.

## Decisión de implementación

1. Continuar el runtime nativo Vita y la reconstrucción incremental de ACT_ONE.
   Ya existe movimiento, combate inicial, terreno, renderizado y transición en
   C; reemplazarlos ahora por un dynarec Win32 sería rehacer el alcance actual.
2. Mantener el modelo de datos externo inspirado por ambas referencias: el VPK
   contiene código propio; los recursos del propietario se preparan aparte en
   `ux0:data/Vagante2Vita/world/`. `tools/prepare_world_bundle.py` verifica el
   paquete antes de la copia al entorno de prueba.
3. Usar Ghidra y el PDB para reconstruir las reglas que aún faltan y contrastar
   cada función contra evidencia local. La traducción estática puede evaluarse
   como una línea de investigación aparte solo si el runtime nativo no alcanza
   el objetivo; requeriría inventario de imports, traducción de instrucciones,
   equivalencia, memoria y servicios de sistema completos.
4. No usar el extractor MPQ de D2Vita para `.vra`: son formatos distintos. Se
   puede estudiar su separación entre núcleo genérico, adaptadores de
   plataforma, herramientas de formato y datos privados, sin copiar código ni
   asumir compatibilidad entre juegos.

## Próximo cuello de botella del mundo

La extracción de recursos está disponible; no es el bloqueo para ACT_ONE. El
runtime aún marca los cofres como abiertos sin crear objetos. La pasada
enfocada de Ghidra recuperó `ItemGenerator::generateNewRandomChestItem`
(`0x006434f0`): consume un sorteo del RNG, selecciona `POOL_CHEST`, llama a
`getRandomItemTypeFromPool` y construye el resultado con
`generateNewRandomItemOfType`. `Chest::finishOpen` (`0x00546210`) llama esa ruta
cuando el cofre no tiene contenido y agrega el resultado al nivel. La llamada
incluye nivel/bonificador del cofre y `magicFindModifier`; sus campos todavía
requieren contraste con las estructuras PDB antes de portarlos.

También están exportadas localmente las funciones de selección de pool
(`0x00645540`) y creación/modificadores del objeto (`0x006435d0`), más el
constructor del generador. Eso reduce el problema desde “descubrir el loot” a
“traducir el camino completo con layouts y RNG correctos, y agregar entidades
Item/inventario al runtime”. El pseudocódigo Ghidra no es fuente recompilable.
No se añade aún loot aleatorio aproximado: el siguiente cambio debe tener
fixtures que cubran pool, nivel, descarte de objetos únicos, estado del cofre y
recogida del objeto.

La reconstrucción no se declara terminada hasta demostrar generación fiel,
entidades activas, loot, trampas, combate, salida y transición con recorrido
físico; Vita3K debe completar una replay cuando se resuelva su fallo de
arranque en este equipo.
