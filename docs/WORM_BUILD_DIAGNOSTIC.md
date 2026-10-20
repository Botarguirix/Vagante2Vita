# Worm integration build diagnostic

The provisional Act One Worm integration is host-testable and now produces a
structurally valid VPK with size optimization. The default `-O3` build still
crashes in the Windows VitaSDK converter; the `-Os` build is the current test
artifact. Vita3K startup validation remains incomplete.

## Reproduction

From the repository root, run:

```powershell
.\tools\build_vpk_windows.ps1 -Iteration 163 -OutputDirectory _local/build-worm-vpk163b
```

Compilation and linking complete, but `vita-elf-create.exe vagante2vita
vagante2vita.velf` exits with Windows status `0xC0000374` (heap corruption).
The failure also reproduces when called directly with `-n` and `-p`; neither
option changes the result. The failing executable is a valid ARM ELF according
to `arm-vita-eabi-readelf`, with three program headers and ordinary `.text`,
`.data`, and `.bss` segments. The known-good baseline at
`_local/build-dragon-actions-163-final/vagante2vita` converts successfully with
the same installed tool. Changing only the build optimization to `-Os` allows
`vita-elf-create`, `vita-make-fself`, and VPK packaging to complete:
`_local/build-act-one-replay2048-os163/Vagante2Vita-iteration163.vpk`.
The installed `check_vpk.py` reports 7 entries, 0 errors, and 0 warnings.

The current ELF has 4,453 `.rel.text` entries versus 4,395 in the baseline, and
2,518 symbol entries versus 2,503. Verbose tool output ends while enumerating
`.rel.text` around address `0x810399fc`; binutils can still read the full table.
Reducing `.rel.text` in temporary ELF copies did not avoid the crash, while the
smaller `-Os` link succeeds. The evidence points to a size/layout-sensitive
Windows host-tool failure, but does not identify a corrupt relocation
conclusively.

The Worm path history was moved out of `World` after discovering that route
probes copy `World` by value. This prevents every simulated world copy from
carrying a 512-point history. It did not resolve the ELF conversion crash. The
history now uses four address-keyed static slots; if more simultaneous worlds
need Worm simulation, replace this with an explicit per-instance runtime store.

## Validation status

- `tools/run_host_tests_windows.ps1 -OutputDirectory _local/host-tests-worm-path-table`:
  13 C test groups pass.
- `python -m unittest discover -s tests -q`: 207 tests pass after the
  path-history move.
- Replay limit raised to 2,048 steps; current three-floor physical route replay
  is 1,178 steps / 8,979 nominal ticks, with all three route proofs freshly
  generated against the current simulator sources.
- The VPK and 142-file ACT_ONE resource package pass their preflight checks.
- Vita3K launch was attempted with the isolated clean profile and a complete
  2,600-file / 3.1 GB copied profile. Both clones were hash-verified before
  staging. The app process remained alive but created no window, emulator log,
  or native session log; the smoke checker therefore fails at
  `before_native_session`. The emulator runtime is hash-pinned in each run's
  `run.json` for follow-up.
- WSL fallback: unavailable in this Windows session (`wsl.exe` reports
  `E_ACCESSDENIED` while enumerating/starting a distribution).

Next diagnostic step: resolve Vita3K's pre-session startup failure against the
isolated copy and then run the 1,178-step campaign replay. Separately, compare
`-O3` and `-Os` linker outputs to determine which section/layout change triggers
the Windows `vita-elf-create` crash. Do not alter VitaSDK-generated ELF
metadata manually as a workaround without validating the resulting VPK in
Vita3K and on hardware.

## Retry from `C:\Vita3k` (2026-10-18)

The user's requested installation at `C:\Vita3k` was retried without modifying
its files or configured profile. The harness copied the 141-file emulator
directory and all 2,600 files in its configured profile to a fresh project-local
test root, then verified SHA-256 equality before staging the VPK and private
campaign resources. The 1,178-step, three-floor campaign replay and 142-file
resource package passed preflight.

Two launches of the isolated copy both failed before a native app session was
created. The first used Vulkan with the Qt frontend; the second matched the
source configuration's OpenGL backend and used the SDL frontend with a visible
window. Both processes remained alive until the harness timeout, created no
window title or `vita3k.log` output, and left stdout/stderr empty. The second
process also showed near-zero CPU use. This rules out the previous renderer
mismatch as the sole cause, but does not identify the host-side startup fault.
Neither run reached the replay or provides evidence about the VPK's behavior in
Vita3K. Run records and checker output are under
`_local/vita3k-from-c-drive-163*/runs/iteration163/run-000001/`.

A follow-up launched the OpenGL clone without any command-line arguments, using
only its colocated `config.yml`; it also stayed alive with no window handle and
no log output. That excludes the harness's frontend/config flags as the sole
cause. The source installation's existing `C:\Vita3k\vita3k.log` contains an
earlier 0.2.1.4194 startup through Qt theme loading and successful Vita3K
initialization, plus firmware extraction. Its log timestamp predates this
follow-up, so it is evidence of an earlier interactive startup, not a
successful test of this VPK or proof that the current automated process reached
the same state. The current user's `C:\Vita3k` process was left untouched.

The current Worm transport record in `BOSS_DECOMP_LEDGER.md` was also corrected:
VWE3 accepts subtype 51 and the runtime creates a provisional 20-segment
visual/body scaffold. Ghidra identifies its Worm-shot projectile and several
attack parameters, but runtime Worm firing/collision, constructor damage,
death/reward, and campaign parity remain unimplemented or unverified.

## Iteration 01.64 update

The bow collision filter now recognizes subtype 51 and checks the provisional
head and all 20 sampled segment hitboxes. Host tests verify arrow hits on the
head and tail, boss-defeat bookkeeping, and reward-chest creation. Segment
shape and damage parity remain unknown. The VPK was rebuilt with `-Os` as
`_local/build-act-one-worm-arrows164/Vagante2Vita-iteration164.vpk` (780,584
bytes, SHA-256
`2653362616d2121b2003fd24f98405f9a4fc8fe6ef575301b2e35727e5eb89de`). Its SFO
identity is `VAGV00006` / `Vagante2Vita` / `01.64`, and the archive contains
only the seven expected VPK entries.

Validation now passes 209 Python tests and 13 C host groups. Vita3K was not
able to reach the 01.64 title: OpenGL/Qt timed out after 75 seconds, and
console mode with the VPK content path timed out after 60 seconds. Both runs
ended before a native session. A detached-title SDL/OpenGL debug launch
captured `0xC0000005` at `ucrtbase.dll+0x25F31` / `Vita3K.exe+0x683965`, with
Qt6Core frames. Evidence is in
`_local/vita3k-c164-4194-sdl-debug.txt`; the VPK has no emulator pass.

The Worm route experiment remains failed. Physical anchor sweeps and a
deterministic route-start candidate did not produce a living player who
defeated the Worm and reached the floor exit. The package's route graph can
connect tiles across passages that player collision cannot traverse. The
probe reuses the baseline spawn-to-exit route and activates ranged combat near
the Worm waypoint; the threshold was raised to 128 px for diagnostics. With
the original `[27,13]` anchor, the player dies to ordinary enemies at tick
1,119, with Worm HP 72.8/80. Moving a copy of the Worm packet to `[38,14]`
produces three hits and 44.8/80 HP, but the player dies at tick 1,004. This
does not establish that `[38,14]` is reachable as a native generated anchor.
Dragon and Goblin King route proofs still pass. The floor-1 proof is missing; see [ITERATION164.md](ITERATION164.md). A later probe-only `PLAYER_ATTACK` pulse also failed to kill ordinary enemies and was removed. `goblin-stab` is the goblin-to-player damage polygon; the player attack path uses `player-stab`, so this experiment does not demonstrate effective melee against goblins.
A probe-only attempt to move the boss waypoint earlier stalled at tick 7,345
with the Worm untouched; it was removed. The Ghidra export confirms candidates
are converted from level tiles to pixel coordinates for constructors, so the
remaining mismatch is provisional room/candidate/RNG state, not a hidden
coordinate offset. Recover `addBoss` call-state and blockers before changing
the anchor algorithm.
## Reintento Vita3K desde `C:\Vita3k` (iteración 01.64)

Se probó el VPK 01.64 desde una copia completa y aislada de esa instalación
(0.2.1.4194), con 141 archivos de runtime y 2,600 archivos de perfil
verificados. El preflight de los 142 recursos de ACT_ONE pasó y el replay
intro se preparó (52 pasos / 1,321 ticks). OpenGL/Qt agotó 75 segundos antes
de crear sesión nativa; los logs del emulador y la consola quedaron vacíos.
Resultado:
`_local/vita3k-c164-intro-opengl-try4/runs/iteration164/run-000001/result.json`.
El proceso del perfil original no se modificó. Este resultado no prueba el
comportamiento del juego: el título no llegó a ejecutarse.

También se probó el modo consola pasando el VPK directamente; agotó 60
segundos sin sesión nativa. Un lanzamiento depurado del clon con SDL/OpenGL y
`--skip-title` capturó `0xC0000005` en `ucrtbase.dll+0x25F31`, con retorno en
`Vita3K.exe+0x683965` y marcos de Qt6Core. El registro reproducible está en
`_local/vita3k-c164-4194-sdl-debug.txt`.

## Reintento desde `C:\Vita3k` y rutas físicas actuales

Se repitió el arranque tomando la instalación fuente en `C:\Vita3k` y
creando un perfil de trabajo dentro de `_local/vita3k-c164-from-C-Vita3k`.
El perfil fue nuevo; no se tocó el `pref-path` configurado en la instalación
fuente. Se usó Vita3K 0.2.1.4194 (SHA-256 del ejecutable
`DFC5C7CBAF161993636B194E7249EB860CBCC7CC6C35D6AEF79EC7A2B30C54FE`) y el VPK
01.64 (`2653362616D2121B2003FD24F98405F9A4FC8FE6EF575301B2E35727E5EB89DE`).
El preflight del paquete ACT_ONE indicó 142 archivos y 93 imágenes, y el VPK
se extrajo a `ux0/app/VAGV00006`. Sin embargo, el proceso aislado permaneció
sin ventana principal y sin log nativo hasta el límite de 120 segundos; no se
creó ninguna sesión de Vagante. Por tanto, este intento tampoco valida ni
refuta el VPK. El registro del intento es
`_local/vita3k-c164-from-C-Vita3k/runs/iteration164/run-000001/`; `run.json`
conserva el SHA del VPK y el perfil dedicado, mientras `process.json` registra
el timeout recuperado al retornar el wrapper.

La prueba física actual se volvió a correr con el bundle de tres pisos
`_local/campaign-worm-world164l`. El controlador actual derrotó al Worm y
alcanzó la salida del piso 1 (semilla 49, 1,495 ticks, 15 HP) y también
derrotó al Goblin King y salió del piso 2 (semilla 4, 5,467 ticks, 87 HP).
Tras la muerte del Worm, se corrigió el controlador de diagnóstico para dejar
de buscar enemigos comunes y reanudar la ruta física original; esto quitó el
atasco que impedía salir del piso 1. La prueba no está completa: Dragon en
piso 0 sigue muriendo a tick 2,143 con 5 HP antes de derrotarlo. Resultados y
trazas están en `_local/routes-act-one-current-164f/`. Esto describe una
limitación del probe de campaña, no una regresión confirmada en los controles
del juego.

La repetición estricta del harness completo en
`_local/routes-act-one-current-165f/` confirma que Worm y Goblin King vuelven a
pasar. Dragon falla con la misma semilla 1: el jugador muere en el tick 2,026
con Dragon a 46.5/65 HP. Las variaciones de control probadas en esta sesión
documentan el límite del probe, no una mejora de runtime: el predictor de arco
consiguió 7 impactos en una ejecución, pero el jugador murió con el Dragon a
4.2 HP; mover el punto de tiro 16 px atrás conservó 67 HP pero todas las flechas
impactaron terreno y el Dragon no recibió daño; las ventanas de evasión y el
ajuste de distancia no produjeron una ruta ganadora. La telemetría de fuente
confirmó que el daño del Dragon se divide entre contacto corporal y bolas de
fuego; los logs temporales se retiraron del código después del diagnóstico.

La prueba global sigue fallando porque el piso 0 no derrota al Dragon. No hay
una prueba de tres pisos completa ni una validación nativa de Vita3K; el intento
desde `C:\Vita3k` sigue deteniéndose antes de la sesión nativa. Los pasos
pendientes siguen siendo estabilizar navegación/combate del Dragon, reejecutar
la campaña de tres pisos con una sola versión de fuentes y resolver el inicio
automatizado del emulador antes de llamar completo al primer mundo.

## Estado posterior de la iteración 01.65

Se retiraron del probe dos heurísticas Dragon sin resultado verificable: la
predicción de caída de flecha y el salto duplicado al quedar cerca del cuerpo.
El harness quedó otra vez en la política vertical de 18 ticks y la evasión
existente. La repetición completa reproducible está en
`_local/routes-act-one-current-165g/`: floor 0 (semilla 1) falla en el tick
2,143, mientras floor 1 (semilla 49, Worm) y floor 2 (semilla 4, Goblin King)
derrotan al jefe y alcanzan la salida. El resultado global es `passed=false`.
No hubo cambios del VPK por esta limpieza del probe, y Vita3K sigue pendiente
porque el arranque desde `C:\Vita3k` no creó sesión nativa.

## Reintento con perfil completo de `C:\Vita3k`

Se volvió a probar el último VPK compilado (01.64) desde la instalación indicada,
esta vez clonando la instalación y su perfil completo en
`_local/vita3k-complete-from-C-Vita3k-165/`. El harness verificó por SHA-256 los
141 archivos de Vita3K y los 2,600 archivos del perfil, sin escribir en el
perfil fuente. El paquete ACT_ONE pasó el preflight (142 archivos, 93 imágenes)
y se preparó la misma replay de entrada (52 pasos / 1,321 ticks).

Con Vita3K 0.2.1.4194, frontend Qt y renderer OpenGL, el proceso de la copia
aislada no creó ventana ni log de sesión durante 120 segundos. El harness lo
detuvo al vencer el límite; `vita3k.log`, stdout y stderr quedaron vacíos. El
informe está en
`_local/vita3k-complete-from-C-Vita3k-165/runs/iteration164/run-000001/result.json`.
Esto vuelve a localizar el bloqueo antes de cargar Vagante y no aporta una
conclusión sobre el VPK. La instancia fuente `C:\Vita3k` y su perfil compartido
no fueron modificados ni cerrados.

## Iteración 01.65: cantidad de bolas del Dragon según Ghidra

La revisión de `BossDragon::step` y `shootFireballsAtPlayer` confirma que las
acciones 6 y 7 pasan 3 y 1, respectivamente, y que la función termina después
de incrementar el contador de proyectiles hasta ese parámetro. El runtime ahora
emite la cantidad solicitada; cada tiro espera un ciclo de 55 ticks, derivado
del sprite de cinco cuadros y velocidad 10. El controlador provisional alterna
ambas variantes porque la secuencia MT19937 nativa aún no está reconstruida.
La velocidad variable, el primer instante exacto de disparo y la dispersión de
puntería continúan pendientes. Véase [BOSS_DECOMP_LEDGER.md](BOSS_DECOMP_LEDGER.md).

La prueba C host pasó sus 13 grupos y la suite Python pasó 209 pruebas y 70
subtests. La campaña física se repitió en `_local/routes-act-one-current-165h/`:
Worm y Goblin King derrotan al jefe y alcanzan la salida; el Dragon mata al
jugador en el tick 1,600 con 15 HP al tick 1,500. El resultado global sigue
`passed=false`, por lo que esta iteración corrige el conteo del ataque pero aún
no completa el primer mundo ni prueba que el combate sea superable.

El VPK 01.65 se compiló con VitaSDK (`-Os`) en
`_local/build-act-one-dragon-volley165/Vagante2Vita-iteration165.vpk`.
`check_vpk.py` valida `VAGV00006`, `APP_VER=01.65`, siete entradas y cero
errores/avisos; tamaño 780,429 bytes, SHA-256
`5152399FF3885DFE4A70891DD7441FD8DF87F8BA92D4A61E1365F7A0673902BA`.
Todavía no se ejecutó el 01.65 en Vita3K: la instalación `C:\Vita3k` y su copia
aislada no inicializaron una ventana/log de emulación, incluso el ejecutable
clonado quedó sin ventana principal al invocarlo con `--help`.

## Reintento desde `C:\Vita3k` y táctica del Dragon

Se intentó de nuevo con el ejecutable solicitado, manteniendo un perfil aislado
en `E:\VaganteVita-Test\vita3k-isolated-165`. El modo de copia completa no
alcanzó el lanzamiento porque `C:` se quedó sin espacio; el runtime ligero,
el firmware y el perfil de Vagante sí quedaron preparados en `E:`. Con Qt
visible, Vita3K mostró el título de ventana `Vita3K v0.2.1 4194-b4e35ed3`, pero
el prólogo expiró a los 60 s antes de crear log de emulador o sesión nativa.
El modo consola con el VPK como ruta de contenido produjo el mismo resultado.
Los informes están en `E:\VaganteVita-Test\vita3k-isolated-165\runs\iteration165\run-000003\result.json`
y `run-000004\result.json`; ambos registran
`failure_stage=before_native_session`. El perfil global configurado por
`C:\Vita3k` no se modificó.

La entrega de trabajo en `campaign-delivery` pasa el validador gráfico (142
archivos y 93 imágenes), pero sus paquetes de enemigos no contienen Dragon en
el piso 0 ni Worm en el piso 1. Para la ruta de tres jefes se usó el fixture
completo `_local/campaign-worm-world164l`. El runner de rutas se ajustó para
aceptar una campaña fuente dentro del repositorio y seguir limitando las
salidas a `_local`.

Se modificó la táctica del probe para conservar más distancia horizontal al
Dragon durante la carga del arco y evitar caminar hacia el cuerpo mientras
apunta. En `_local/routes-act-one-current-165l/`, los pisos Worm y Goblin King
pasaron; el jugador murió en el tick 2,690 con 5 HP tras haber dejado al Dragon
en 13.5 HP al tick 2,400. El ajuste mejora los impactos pero todavía no
constituye una ruta física superada, así que el proof de piso 0 sigue pendiente.
Las suites host pasan: 13 grupos C y 209 pruebas Python más 70 subtests.

## Reintento usando `C:\Vita3k` como origen (2026-10-18)

El runner `tools/test_vita3k.ps1` tomó `C:\Vita3k` como origen y creó un
runtime y un `pref-path` nuevos en
`_local/vita3k-cdrive-rerun-20261018`; no reutilizó el perfil global. Se
instaló el VPK 01.65 en ese perfil y se copiaron 134 archivos de recursos. El
paquete y la campaña pasaron sus preflight (144 archivos, 3 pisos, 3 jefes), y
el binario fuente fue Vita3K 0.2.1.4194 (`DFC5C7CB...B30C54FE`).

La prueba de campaña completa se detuvo antes del lanzamiento porque los proofs
de ruta de `campaign-worm-world164l` fueron producidos por otra versión de las
fuentes del simulador. Después se ejecutó la secuencia básica del prólogo con
Qt visible y OpenGL durante 90 segundos. Vita3K no escribió `vita3k.log` ni
creó una sesión nativa; el runner terminó el proceso y registró
`failure_stage=before_native_session` en
`_local/vita3k-cdrive-rerun-20261018/runs/iteration165/run-000002/result.json`.
Los stdout/stderr y los logs de depuración están vacíos. El hash del VPK
probado es `5152399FF3885DFE...673902BA`.

Resultado: el nuevo origen quedó confirmado, pero la ruta automatizada de
Vita3K sigue sin inicializar la emulación en esta máquina. No se pudo afirmar
que el juego ni el prólogo se ejecutaran; la prueba pendiente debe investigar
el bloqueo de arranque antes de evaluar el replay o el combate.

## Reconciliación de proofs y Dragon (iteración 166)

Regeneré los proofs de los pisos 1 y 2 contra las fuentes actuales; ambos
vuelven a pasar y usan el fingerprint
`6b592b4dc175987fe189d6fa20787d717e63e9c36ec718ef6b7dca5ed8fbe0ab`. El
Dragon sigue bloqueando la campaña. La mejor ruta repetida hasta ahora,
`_local/routes-act-one-current-166n/`, lo deja en 14.2 HP al tick 3,600 con
15 HP del jugador, pero el jugador muere en el tick 3,811. Registró 6 impactos
de flecha y 15 impactos en terreno. Su `route-results.json` sigue en
`passed=false`, por lo que no hay proof de piso 0.

Las variantes de piloto delimitan el problema. Activar el combate desde la
distancia al jefe elimina el cuelgue, pero la variante 166a muere en el tick
2,719 con el Dragon en 28 HP; forzar el escape al tocar pared (166c) muere en
el tick 1,572 con 37.2 HP. El ajuste de evasión por proyectil (166j) evita la
muerte temprana, pero el personaje queda en x=424 contra una pared, gasta sus
30 flechas con solo 4 impactos y muere en el tick 5,269 con el jefe en 37.2
HP. Ninguna variante se empaquetó como una ruta completada.

La verificación host volvió a pasar sobre las fuentes actuales: 13 grupos de
pruebas C y 209 pruebas Python más 70 subtests. El siguiente trabajo debe
resolver la posición de disparo y salida del bloqueo del Dragon para obtener
el proof físico del piso 0. Vita3K sigue bloqueándose antes de inicializar una
sesión nativa, según el reintento aislado documentado arriba.

El intento de aumentar la frecuencia de los ataques cercanos de 48 a 30 ticks
fue peor: el jugador murió en el tick 2,425 con el Dragon en 41.2 HP. Se
restauró el intervalo de 48 ticks; el estado fuente actual coincide con la
táctica más efectiva de `_local/routes-act-one-current-166n/`.


## Reintento SDL/Vulkan en `C:\Vita3k` (2026-10-18)

Se probó Vita3K desde `C:\Vita3k` con un `TestRoot` nuevo dentro de `_local`,
frontend SDL, renderer Vulkan y el VPK 01.65. El preflight confirmó el paquete
y los 144 recursos del fixture de tres pisos. La ejecución básica del prólogo
expiró a los 90 segundos sin crear sesión nativa. `result.json` registra
`failure_stage=before_native_session`, `process_timed_out=true`; stdout,
stderr, debug logs y `vita3k.log` están vacíos. El informe queda en
`_local/vita3k-cdrive-sdl-vulkan-20261018/runs/iteration028/run-000001/`.
El intento no demuestra ejecución del VPK. El runner se limitó a su perfil
aislado; no se instalaron archivos en el perfil global.

La ruta física 166p probó reposicionar al piloto hacia el waypoint cada vez que
una baldosa tapaba la línea de visión del Dragon. Fue peor que 166n: murió en
el tick 2,771 con el jefe en 30 HP (16 flechas, 4 impactos y 12 impactos en
terreno). Se retiró esa condición del piloto para conservar la táctica previa
de 166n; las pruebas 166p quedan como diagnóstico, no como proof válido.

Después de ese intento, las 13 suites C pasaron y pytest reportó 209 pruebas y
70 subtests aprobados. El resultado no cambia el estado pendiente: falta una
ruta física que derrote al Dragon, y Vita3K todavía no alcanza una sesión
nativa en esta máquina.

## Arranque completo desde `C:\Vita3k` y corrección del runner (01.65, 2026-10-18)

Se corrigió un error en `tools/test_vita3k.ps1 -CopyCompleteInstance`: la
selección del ejecutable se hacía antes de copiar la instancia y podía elegir
la raíz en lugar de `instance/runtime`. Ahora el runner elige esa ruta a partir
de la instalación fuente después de clonar y valida que el ejecutable exista.
El caso con el runtime 0.2.1.4170 parcial confirma la ruta en `run.json`, aunque
ese fixture no incluía sus DLL Qt junto al ejecutable y Windows lo cerró con
`0xC0000135`; no sirve como prueba de arranque funcional.

Se clonó íntegramente `C:\Vita3k` a
`_local/vita3k-cdrive-complete-165-20261018`: runtime 0.2.1.4194, 141 archivos
de instalación y 2,600 archivos de perfil verificados con SHA-256. El VPK
01.65 (`5152399FF3885DFE4A70891DD7441FD8DF87F8BA92D4A61E1365F7A0673902BA`)
y los 144 recursos de campaña pasaron preflight. Con frontend SDL y renderer
Vulkan, el título quedó sin ventana ni salida y expiró a los 90 segundos, antes
de crear sesión nativa. El mismo resultado aparece con Qt/OpenGL y con una
prueba de SDL sin título: el bloqueo es previo a la carga del VPK.

El depurador de Windows reprodujo la excepción sin cargar juego:
`0xC0000005` en `ucrtbase.dll+0x25F31`, retorno en `Vita3K.exe+0x683965` y
marcos de `Qt6Core.dll`. El registro de este clon está en
`_local/vita3k-cdrive-complete-165-20261018/runs/iteration165/run-000001/debug-skip-title.txt`.
Esto señala el frontend/configuración Qt de la build 4194 como foco; no prueba
ni refuta el runtime de Vagante. El perfil original de `C:\Vita3k` quedó sin
cambios.

Validación tras el cambio del runner: el script PowerShell pasa el parser, los
3 casos del harness de fallos Vita3K pasan, las 13 suites C host pasan y pytest
pasa 209 pruebas más 70 subtests. La compilación VPK y la ruta física del
Dragon no cambiaron en esta iteración; el primer mundo sigue sin proof del
Dragon y pendiente de una ejecución nativa de Vita3K.

## Línea de visión del Dragon: experimento 166q

El CSV del piloto 166n se conservó en
`_local/route-dragon-166n-inspect/dragon-route.csv` (muestras cada 12 ticks).
Confirma que varias flechas se gastan contra terreno, pero también que la salud
del jugador cae mientras se aproxima y espera. La variante 166q evitó iniciar
nuevos tiros sin línea directa y mantuvo cargado el arco hasta recuperar
visibilidad. Fue peor: el jugador murió en tick 2,021 con el Dragon a 46.5 HP,
tras 3 impactos y 5 flechas detenidas en terreno. Se retiró el cambio.

Una repetición 166r con la táctica restaurada reprodujo el resultado de 166n:
Dragon a 14.2 HP y jugador muerto en tick 3,811; pisos 1 y 2 vuelven a producir
proofs físicos. El fingerprint del simulador regresó al mismo valor de 166n,
`6b592b4dc175987fe189d6fa20787d717e63e9c36ec718ef6b7dca5ed8fbe0ab`. El piso 0
sigue sin prueba de victoria. El siguiente ajuste debe buscar daño seguro
adicional durante las aperturas de ataque del Dragon, sin esperar detrás del
obstáculo ni cambiar la ruta a ciegas.

## Rutas físicas completas y nuevo intento Vita3K (166x, 2026-10-18)

La variante 166x consiguió por primera vez completar físicamente las rutas de los tres pisos en el simulador nativo. Los proofs versionados y sus trazas VTR están en `_local/routes-act-one-current-166x/`; los tres registran contacto con la salida y victoria del jefe, con el jugador vivo. El piso 0 tocó físicamente la salida en el waypoint 3 de 6; se acepta porque el solapamiento con la meta real es la evidencia de salida, aunque el contador de waypoints guía no haya terminado. La grabación conserva los controles completos y no salta al piso siguiente.

Se generó `_local/campaign-act-one-complete-166x` a partir de esas pruebas y los datos de pisos existentes. El manifiesto pasa validación, contiene los tres pisos y enlaza proofs reproducibles para cada uno. Esto verifica recorridos en el simulador de control; **no** demuestra paridad de generación con el juego original ni una transición nativa entre pisos. `campaign_complete` y `floor_complete` siguen siendo falsos deliberadamente.

El smoke de campaña usó el VPK 01.65 (`5152399FF3885DFE4A70891DD7441FD8DF87F8BA92D4A61E1365F7A0673902BA`), los recursos preflight y un perfil aislado preparado desde `C:\Vita3k`. Con frontend Qt y OpenGL, el replay de 1,328 entradas se preparó, pero Vita3K expiró a los 90 segundos antes de crear el log de sesión nativo. `vita3k.log`, stdout y stderr quedaron vacíos en `_local/vita3k-cdrive-qt-opengl-166x/runs/iteration165/run-000001/`. La interacción de entrada al mundo y la transición no quedaron verificadas en Vita3K; el fallo ocurre antes de que el juego pueda probarse. La instalación fuente y su perfil compartido no se modificaron.

Después del empaquetado, la validación del manifiesto pasó y las 13 suites C host compilaron y aprobaron en `_local/host-tests-cdrive-after166x/report.json`.

El 18 de octubre también se intentó el modo consola (`--console` con instalación del VPK por ruta de archivo) en un `TestRoot` independiente: el VPK y el replay volvieron a pasar preflight, pero a los 45 segundos no había sesión nativa ni contenido en logs. El resultado está en `_local/vita3k-cdrive-console-166x/runs/iteration165/run-000001/result.json`. Así, los modos Qt/OpenGL y consola fallan antes de probar el título; los procesos activos de la instalación fuente no se terminaron ni modificaron.

Se repitió el lanzamiento de consola usando la copia completa aislada de `C:\Vita3k` (instalación y perfil completos, ya verificados por hash en el experimento anterior), lo que descarta que el timeout de la prueba ligera se deba solamente a las particiones `sa0` u otros archivos omitidos. También venció a los 45 segundos sin salida en stdout/stderr ni en `vita3k.log`; evidencia en `_local/vita3k-cdrive-full-profile-console-166x/`. El binario de `C:\Vita3k` es Vita3K 0.2.1 build 4194; la página oficial de Automatic CI consultada el 2026-10-18 sigue señalando build 4194, así que no había un artefacto upstream más nuevo que pudiera probarse en esta pasada.

## Reintento desde el ejecutable original de C:\Vita3k (2026-10-18)

Se inició `C:\Vita3k\Vita3K.exe` directamente con el VPK 01.65 y el argumento
`--config-location` apuntando al perfil de la copia aislada del proyecto. El
proceso de prueba se cerró al vencer 20 segundos; `stdout.log` y `stderr.log`
quedaron vacíos y no apareció un log nuevo en la carpeta de trabajo. Como el
log de instalación tenía una fecha anterior al intento, este resultado no
permite verificar qué ruta de `VitaFS` habría usado ni confirmar que el perfil
fuera realmente aislado. Los dos procesos Vita3K que ya estaban abiertos se
dejaron en ejecución. No se obtuvo una sesión nativa ni evidencia de gameplay;
la opción de configuración debe validarse antes de usar este ejecutable para
una prueba que pueda escribir en sus datos globales.

## Corrección de estadísticas base del Worm

La exportación Ghidra de `BossWorm::initializeStats` (`0x00530df0`) confirma
100 HP y daño nominal 3 con los factores por defecto 1.0; NG+ usa 500 HP y
daño 10. `BossWorm::doCollision` (`0x00531d00`) aplica daño nominal −1 más un
rollo uniforme 0–2 y 30 frames de cooldown por jugador. El runtime se actualizó
de 80 a 100 HP y usa 2–4 como modelo de contacto. Los resultados de ruta de la
iteración 01.64 siguen siendo evidencia histórica con 80 HP; hay que volver a
ejecutarlos con el nuevo límite antes de conservarlos como prueba actual.

## Diagnóstico de inicio del emulador (18 de octubre de 2026)

Se repitió con el frontend Qt/OpenGL visible y un perfil aislado derivado de
`C:\Vita3k`. El diálogo del sistema informó una lectura en dirección nula. El
capturador de excepciones confirma una violación de acceso en
`ucrtbase.dll+0x25F31`, llamada desde `Vita3K.exe+0x683965`; también ocurre al
iniciar el frontend sin cargar Vagante (`--skip-title`) y al omitir las opciones
`--load-config`, `--keep-config` y `--config-location`. Esto descarta el VPK, la
replay, los recursos del ACT_ONE y esos argumentos de configuración como causa
necesaria del fallo. El smoke administrado termina sin sesión nativa, antes de
que el título se ejecute. No se deben interpretar estas corridas como un error
del juego.

El perfil de prueba usa el binario y DLLs copiados desde `C:\Vita3k`; las dos
instancias Vita3K que ya estaban ejecutándose desde esa ubicación y su perfil de
Touhou quedaron fuera de la prueba. La dirección RVA es evidencia para buscar
la función de Vita3K, todavía no identifica por sí sola el origen lógico del
puntero nulo. El siguiente paso útil es comparar la misma compilación iniciada
por su interfaz normal y obtener una traza simbólica/PDB para `Vita3K.exe`.

## Reintento con la instalación C:\Vita3k y el paquete 01.66 (2026-10-18)

Se compiló un VPK local de los cambios actuales, sin reemplazar el paquete de
entrega: `_local/build-c-vita3k-166/Vagante2Vita-iteration166.vpk`, SHA-256
`E896357293702B8EE96B06B5A3106CA89144DA6EB5F2C216D32391BB52F24E3A`. Se
usó el arnés de perfil aislado con fuente `C:\Vita3k`; el ejecutable copiado
coincide con el original, SHA-256
`DFC5C7CBAF161993636B194E7249EB860CBCC7CC6C35D6AEF79EC7A2B30C54FE`.
Con Vulkan y OpenGL, cada ejecución agotó 45 segundos antes de crear un log de
sesión nativo. `result.json` reporta `failure_stage=before_native_session`,
`process_timed_out=true`, `emulator_log_available=false`; por tanto, el VPK
no llegó a ejecutarse y el resultado no valida ni invalida gameplay. El intento
OpenGL inicial no alcanzó el lanzamiento por espacio insuficiente; se liberaron
solo los perfiles temporales creados para esta prueba y luego se repitió con
espacio disponible. No se modificaron ni cerraron las instancias originales.

## Ruta ACT_ONE con spit activo: regresión confirmada (2026-10-18)

La ruta previa de floor 1 (`routes-worm-stats-20261018`, 1,597 ticks, 36 HP)
se obtuvo antes de activar el nuevo scheduler de spit; no es proof del código
actual. La reproducción con scheduler activo llega al jefe pero se desvía o
muere por daño acumulado. Al desactivar únicamente el scheduling en un binario
de diagnóstico, las tres rutas vuelven a pasar; esto confirma que el cambio de
combate afecta a la ruta automatizada y que los proofs anteriores no bastan
para validar esta versión.

Se probaron variantes del controlador de prueba: disparo a distancia condicionado
por línea de visión, acercamiento melee, esquiva de proyectiles entrantes,
ataques contra segmentos del Worm y búsqueda de puntos para descender durante
la pelea. Las ejecuciones más recientes siguen fallando floor 1 (muerte o
timeout al bajar a la siguiente plataforma), mientras floors 0 y 2 pasan. No
se generó un proof nuevo para floor 1. Las 13 suites host sí pasan en
`_local/host-tests-worm-dodge-20261018/report.json`, incluidos spawn, movimiento
y contacto del proyectil; esas pruebas unitarias no prueban que la campaña sea
completable con el spit activo. La siguiente corrección debe resolver la
travesía y la respuesta del jugador a los ataques sin ocultar el proyectil ni
reciclar el proof de 166x.

## Reintento de navegación física (2026-10-18)

El arnés de campaña volvió a fallar floor 1 en el waypoint 27/93; floors 0 y
2 pasan. En ese punto el jugador queda en `(338.13, 170.00)` con `WORLD_RIGHT`
bloqueado, y el Worm conserva 100 HP. El registro de posiciones indica que la
ruta calculada por tiles no describe la transición física hacia la zona bajo
el techo/plataforma. La corrida está en
`_local/routes-worm-platform-drop-any-20261018/`.

Se probó un grafo de navegación que simulaba aristas con `player_step`, pero
solo conectó seis nodos desde el spawn y no llegó a la salida. No se usa como
validación ni como reemplazo de la ruta existente. Se retiró el grafo de la
fuente; la instrumentación y suites host no convierten floor 1 en una prueba
aprobada. Las 13 suites host pasan en
`_local/host-tests-route-investigation-20261018/report.json`. El smoke de
Vita3K desde `C:\Vita3k` sigue venciendo antes de crear una sesión nativa; no
hay evidencia de ejecución del VPK.

## Reintento de navegación de floor 1 (2026-10-18)

Se corrieron nuevamente las tres rutas con el scheduler de spit activo en el
paquete de campaña `campaign-act-one-wormstats-166`. Floors 0 y 2 completan
sus pruebas; floor 1 sigue fallando en el waypoint 27 de 93, con el jugador
estable en `(338.13, 170.00)`, vida 77, `WORLD_RIGHT` bloqueado y Worm con 100
HP. El log está en `_local/routes-worm-platform-drop-any-20261018/floor-01.log`.
El mapa contiene un techo y una plataforma estrecha en la región x=21..27,
y=11..14. No hay un proof nuevo de floor 1; los proofs históricos no deben
representar el código actual.

Se inspeccionaron alternativas de caída, pero se descartaron los cambios
experimentales del controlador porque no lograron salir del waypoint. Se
conservaron las modificaciones preexistentes del controlador y se retiraron
las condiciones añadidas durante este diagnóstico. Los intentos Vita3K desde
`C:\Vita3k` siguen fallando antes de abrir la sesión nativa, por lo que no
validan el VPK. El siguiente paso debe usar un grafo de navegación física
(saltos, escaleras y caída de plataformas), en lugar de aproximar la caída
desde la distancia Manhattan del path de tiles.

## Floor 1: navegación por plataforma y Worm derrotado (01.67, 2026-10-18)

La causa del loop fue que una etapa de caída se marcaba como terminada al
pasar cerca de su coordenada mientras el jugador aún estaba en el aire. El
controlador volvía a saltar a la pared superior y repetía la caída, recibiendo
daño de los proyectiles. La etapa ahora permanece activa hasta que el jugador
aterriza; el pathfinder elige una plataforma inferior al mismo nivel del
waypoint y fuerza el movimiento horizontal después del aterrizaje. Al agotar
las tres flechas de la clase Arquero, el controlador se acerca para atacar con
melee y mantiene el waypoint del jefe bloqueado hasta derrotarlo.

La prueba física ACT_ONE completa pasa en los tres pisos con el scheduler de
spit activo. Resultado y pruebas reproducibles en
`_local/routes-worm-campaign-melee-finish-20261018/route-results.json`; floor
1 (seed 49) derrota al Worm, llega a la salida y termina con 14 HP en 1,650
ticks. Floors 0 y 2 también pasan. Este resultado valida el modelo de combate
y navegación del simulador host, no paridad completa con la generación ni el
render del juego original. Las 13 suites host deben correrse junto con estos
proofs.

El intento de WSL para compilar fue rechazado por Windows con
`Wsl/Service/CreateInstance/E_ACCESSDENIED`; para producir el paquete 01.67
se usó el VitaSDK de Windows disponible en `C:\vitasdk`. Vita3K desde
`C:\Vita3k` sigue pendiente de validación de gameplay porque las pruebas
anteriores vencieron antes de crear una sesión nativa.

## Vita3K no-title startup crash — 01.97 (2026-10-18)

The full-campaign harness run using Vita3K 0.2.1.4194 from `C:\Vita3k` timed
out before producing a native session log. To separate emulator startup from
the VPK, the same isolated runtime was started under
`tools/debug_vita3k_windows.py` with `--frontend sdl --skip-title`; no game or
Vagante resources were loaded. The debugger caught a first-chance and then
unhandled access violation at `ucrtbase.dll+0x25F31`, with a return address at
`Vita3K.exe+0x683965` and Qt6Core frames at `+0x3F11A0` and `+0xAE1F0`. The
process exited with `0xC0000005`.

The run used the project-local isolated copy at
`_local/vita3k-act-one-197-campaign/runtime/Vita3K.exe`; the configured
`C:\Vita3k` installation and its profile were not changed. The executable hash
is recorded in that run's `run.json`. Since the crash reproduces with no title
loaded, the 01.97 campaign timeout cannot be interpreted as a VPK failure or
gameplay result. Vita3K startup must be repaired or replaced before its runtime
can validate the campaign.
