# Reproducir el análisis

La evaluación de las rutas de recompilación y extracción de recursos basada en
`repentogxm` y D2Vita está en [PORTING_REFERENCE_REVIEW.md](PORTING_REFERENCE_REVIEW.md).

## VPK actual (iteración 039)

El VPK fuente listo para prueba está en
`_local/build-iteration039/Vagante2Vita-iteration039.vpk`. VPK de 808661 bytes,
SHA-256 `1FBA9C53E5B2110ED2397684C693DBF07A5E6ED4B4890400FAF15AEBC1123BE8`.
El smoke Vulkan/SDL esperó 30 s y terminó con código -1, sin `vita3k.log` ni
sesión nativa; en 038, OpenGL/SDL, Vulkan/SDL y Vulkan/Qt dieron el mismo
síntoma. Véase
[ITERATION39.md](ITERATION39.md) y el diagnóstico [ITERATION38.md](ITERATION38.md).

## Smoke ACT_ONE anterior (iteración 031)

El test corre contra una copia privada del emulador y filesystem bajo `_local`;
no modifica el Vita3K usado por otros proyectos. Requiere el paquete privado
`_local/delivery-045` y el VPK 00.31:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/test_vita3k.ps1 -Action Run -Iteration 031 -EmulatorDirectory _local/vita3k -TestRoot _local/vita3k-vagante-clone -Vpk _local/build-act-one-031/Vagante2Vita-iteration031.vpk -ResourceDelivery _local/delivery-045 -ActOneExitSmoke -TimeoutSeconds 45
```

El harness registra VPK/hash, guarda stdout/stderr/logs y cierra sólo el proceso
del clon si excede el timeout. Resultado observado en este entorno: Vita3K
0.2.1.4183 no produjo ventana ni log nativo después de 45 s; el smoke quedó sin
ejecución de gameplay. Ese fallo de arranque no prueba una regresión del juego.
El timeout corto (5 s) verifica la ruta de limpieza; sólo una corrida con log
nativo y aserciones verdes validará el recorrido.

Usar una copia propia de Vagante. Python 3.10+ para inspectores; Windows PowerShell para el lanzador Ghidra.

1. Ejecutar `tools/inspect_game.py GAME_DIRECTORY`. Produce `_local/analysis/binary.json` y `symbols.json`. El PDB debe coincidir con GUID y age del EXE antes de usar direcciones y tipos. `python tools/triage_symbols.py` genera un inventario local de funciones por subsistema.
2. Ejecutar `tools/vra.py GAME_DIRECTORY/data.vra`. Produce inventario con hashes y tamaños. `--extract _local/assets` extrae recursos a un directorio nuevo y preserva duplicados en `_duplicates/ORDINAL/`. No repaqueta ni modifica Steam. Un directorio con archivos existentes causa error; usar otro directorio para repetir.
3. Ejecutar `tools/setup_re.py`. Descarga Ghidra 12.1.4 y JDK 21.0.12.1 oficiales en `_local/toolchain`; requiere red y espacio libre. Verifica SHA-256 de Java y el digest de Ghidra cuando GitHub lo ofrece. Las herramientas descargadas no se publican. Python 3.11+ es necesario para este instalador.
4. Ejecutar `./tools/run_decompile.ps1 -GameDirectory 'RUTA'`. Copia EXE/PDB a `_local/input` para evitar problemas del lanzador batch con paréntesis en la ruta de Steam. Importa y analiza, luego exporta con `tools/ghidra/ExportDecomp.java`. Puede escribir configuración de Ghidra en el perfil de usuario. El proyecto local Vagante se sobrescribe al repetir.
5. Revisar `_local/ghidra/analysis.log`, `_local/decompiled/functions.tsv`, `types.tsv`, `fields.tsv` y `summary.txt`. Los dos inventarios de tipos conservan layouts Windows observados; no son headers C++ portables. Si el análisis agota 1 800 segundos o hay advertencias de tipos/flow, las funciones correspondientes quedan sin validar. El límite por función de decompilación es 30 segundos. `-MaxFunctions N` limita la exportación; 0 intenta todas las funciones internas reconocidas.

Cada `.c` exportado está identificado por dirección y contiene pseudocódigo. Los nombres/tipos PDB ayudan, pero offsets, excepciones, RTTI y llamadas MSVC requieren revisión. No anunciar resultados como código original ni fuente ARM lista para compilar.

## Paquete externo de mundo

El runtime lee el mundo y los PNG desde `ux0:data/Vagante2Vita/world`; el VPK no necesita incluir los recursos comerciales. Para preparar esa carpeta, usar una copia propia del juego y las salidas locales del análisis:

```powershell
python tools/prepare_world_bundle.py _local/assets _local/generation036/map-export/act-one-world.vwi _local/generation036/map-export/intro-terrain.png _local/world-pack
python tools/prepare_world_bundle.py --validate _local/world-pack
```

Para preparar el ACT_ONE provisional actual (no representa aún un piso completo), con sprites y entidades exportadas, usar:

```powershell
python tools/prepare_world_bundle.py _local/assets _local/generation045/map-export/act-one-world.vwi _local/generation045/map-export/intro-terrain.png _local/world-data045/act-one --act-one --entity-packet _local/generation045/map-export/act-one-entities.vwe --attacks _local/assets/attacks.json
python tools/prepare_world_bundle.py --validate _local/world-data045/act-one
```

El paquete actual ACT_ONE también necesita la tabla privada de cofres y sus
atlas: genera `item-sprite-indexes.json` desde Ghidra/enum y compílalo con
`tools/compile_item_loot.py`; después pasa `--loot-tables
_local/item-loot.vlt` al preparador. El runtime 062 exige ese archivo para
cargar ACT_ONE. Véase [ITERATION62.md](ITERATION62.md).

El preparador crea `manifest.json` con versión, tamaño y SHA-256 por archivo, e incluye PNG privados de cofres, pinchos, trampas y doodads de ACT_ONE. El runtime dibuja registros VWE2; los cofres se marcan visualmente abiertos con ↑ al tocarlos. Bloqueo/loot, daño y comportamiento de trampas, y animación fiel siguen pendientes. El layout tiene paridad de generación falsa y la ruta A* no representa física. El exportador escribe `floor_complete=false` y rechaza marcarlo completo sin paridad, entidades instanciadas, recorrido físico probado y transición de salida comprobada. Copiar la carpeta resultante como `world/act-one/` en la entrega de Vita3K/Vita. La instalación del pack es manual vía VitaShell o Vita3K; el runtime de Vita todavía no lee el manifiesto y no realiza hot reload. El verificador de escritorio detecta archivos alterados y rutas inseguras. Mantener este paquete privado en `_local`; no incluirlo en Git ni en el VPK.

El criterio para llamar “completo” al primer mundo incluye piso reproducible por semilla, ensamblado definitivo con terreno y colisiones, entidades activas, cofres/loot, trampas/daño, combate, salida y transición al siguiente piso. Debe existir una prueba host que recorra el mundo usando la misma física `player_step`, y una replay de punta a punta en Vita3K y hardware. Actualmente el bundle sigue marcando `floor_complete=false`; el informe de la iteración 028 documenta los bloqueos y ejecuciones de emulador.

`./tools/retry_decompile.ps1` reintenta sólo los fallos con 120 segundos por función, sin repetir análisis, y actualiza `functions.tsv`; el resumen de la primera pasada permanece en `summary.txt` y el reintento se registra por separado. `python tools/package_analysis.py` comprueba que cada exportación exitosa tiene archivo y genera `_local/Vagante-decompilation.zip` más `verified_summary.json`. Ninguna de estas comprobaciones valida equivalencia de comportamiento.

Para verificar las categorías superiores de loot recuperadas con Ghidra, genera
una listing de `ItemGenerator::ItemGenerator` en la copia privada del proyecto
y compara las referencias float32 contra el ejecutable local:

```powershell
python tools/audit_chest_pool.py --exe _local/input/vagante.exe --listing _local/item-generator-ctor-listing.txt
```

El resultado esperado es `verified: true`, cuatro categorías de cofre con
pesos 30/60/4/6, siete categorías de arma con peso 1.0 cada una y seis
categorías de armadura con pesos 0.7/1.0/1.1/1.0/1.0/0.75. Esto valida solo
esas tres tablas y los cinco tipos/rangos/pesos del subpool de espadas; no
genera loot ni comprueba atributos o capacidad de inventario.

## Pruebas

`python -m unittest discover -s tests -v` verifica truncamiento, rutas peligrosas, límites y duplicados del extractor con archivos sintéticos. La ejecución real del inspector y extractor contra el juego verifica compatibilidad de esta copia; no se sube al CI.

En Windows, si LLVM y MSYS2 UCRT están instalados pero no hay MSVC, las pruebas
C host se pueden compilar y ejecutar con el runtime UCRT configurado por el
runner local:

```powershell
tools/run_host_tests_windows.ps1
```

Por defecto detecta `C:\Program Files\LLVM\bin\clang.exe` y
`C:\msys64\ucrt64`; admite `-Clang` y `-MsysUcrt` para otras rutas. Guarda los
binarios y `report.json` bajo `_local/host-tests-windows`. Esta ejecución cubre
las 12 pruebas C definidas para HOST_PROBE; no sustituye VitaSDK ni valida el
arranque o el renderer en Vita3K.

Con `tools/run_act_one_route_probe.ps1`, `act-one-route-probe` recibe `act-one-world.vwi`, `act-one-entities.vwe` y una semilla y simula el movimiento con `player_step` hasta tocar la salida o agotar 180000 ticks. El runner busca clang/gcc/cc o acepta `-Compiler`, compila las fuentes C reales con warnings estrictos y guarda el ejecutable en `_local/host-route`. Ejemplo: `tools/run_act_one_route_probe.ps1 -World _local/world-data045/act-one/act-one-world.vwi -Entities _local/world-data045/act-one/act-one-entities.vwe -Seed 12345`. Un resultado `reached_exit=1` verifica la ruta bajo la física Vita implementada; no verifica por sí solo paridad del generador, combate/loot, una transición de piso o Vita3K. También está disponible el build CMake host con `-DHOST_PROBE=ON`.


Para repetir el smoke ACT_ONE con una copia aislada de Vita3K y el paquete
privado más reciente:

```powershell
tools/test_vita3k.ps1 -Iteration 028 -EmulatorDirectory 'G:\Vita3k' -TestRoot _local/vita3k-vagante-clone -Vpk _local/build-act-one-028/Vagante2Vita-iteration028.vpk -ResourceDelivery _local/delivery-045 -ActOneExitSmoke -Visible
```

Para el harness actual, usar la entrega de campaña como raíz de paquete y
`-InstallVpkViaCli` para que Vita3K reciba el VPK como argumento de contenido
(en lugar de extraerlo manualmente y ejecutar el título por ID):

```powershell
tools/test_vita3k.ps1 -Action Run -Iteration 053 -EmulatorDirectory _local/vita3k-vagante-4170/runtime -TestRoot _local/vita3k-act-one-053 -Vpk _local/build-act-one-053/Vagante2Vita-iteration053.vpk -ResourceDelivery _local/act-one-campaign050 -InstallVpkViaCli -Frontend sdl -Renderer OpenGL -TimeoutSeconds 90
```

Este modo copia solo los archivos de runtime del paquete ACT_ONE al sandbox de
Vita3K; el manifiesto verifica el resto sin incorporarlo al VPK. En 0.2.1.4183
la ejecución local actual agotó el tiempo antes de instalar el título o crear
una sesión nativa. La FAQ oficial documenta `.vpk` como argumento de contenido
para instalar desde la CLI: [Vita3K FAQ](https://vita3k.org/faq).

El harness sincroniza cada lanzamiento los binarios/DLL y los recursos estáticos
faltantes de runtime, pero conserva separados el filesystem, las preferencias y
cachés. Los intentos 028 con Vita3K 0.2.1.4183/4170 fallan antes de ventana/log
nativo con un error de lectura nula de Vita3K.exe en el Visor de eventos de
Windows; cambiar Vulkan por OpenGL no lo resolvió. Son fallos del emulador y no
resultados de gameplay del VPK. Véanse `_local/vita3k-vagante-clone/runs/iteration028/`
y `_local/vita3k-vagante-4170/runs/iteration028/`.
En Windows con VitaSDK instalado en `C:\vitasdk`, se puede compilar y empaquetar
el VPK sin CMake con `tools/build_vpk_windows.ps1 -Iteration 029`. Los objetos,
SELF y VPK permanecen en `_local/build-act-one-029`; la compilación no agrega
recursos comerciales al VPK. Este flujo directo verifica el compilador y el
empaquetador del SDK, pero las pruebas de gameplay aún requieren ejecución en
host o Vita3K.

El smoke de salida ACT_ONE para 029 usa la copia privada de recursos y el perfil
aislado de Vita3K:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/test_vita3k.ps1 -Action Run -Iteration 029 -EmulatorDirectory _local/vita3k -TestRoot _local/vita3k-vagante-clone -Vpk _local/delivery-029/Vagante2Vita-iteration029.vpk -ResourceDelivery _local/delivery-029 -ActOneExitSmoke
```

El harness deja runtime, preferencias y filesystem de Vita separados de la
instancia en `G:\Vita3k`. La ejecución local terminó con `0xC0000005` en
Vita3K antes de producir una ventana/log nativo; sigue pendiente una corrida de
gameplay cuando se resuelva el arranque del emulador.

## GitHub

El repositorio solicitado es `Botarguirix/Vagante2Vita`. `tools/create_github_repo.py` permite crearlo utilizando Git Credential Manager y GitHub REST; no imprime ni guarda credenciales. Publicación de fuentes propias mediante Git. Antes de publicar, revisar archivos rastreados y verificar que `_local` permanece ignorado.
