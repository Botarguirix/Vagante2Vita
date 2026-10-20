# Iteración 01.35: derrota y recompensa provisional del Dragon

El listing local de Ghidra para `BossDragon::hurt @ 0x0050d780` muestra que,
cuando el golpe letal resulta en `DID_HURT`, llama a
`Level::spawnBossReward @ 0x006594e0` y después a
`Level::removeActiveBoss`. El runtime ahora permite que melee y flechas reduzcan
los 65 HP confirmados del Dragon y, al derrotarlo, incrementa el contador,
detiene su movimiento y crea una caja de recompensa por la misma ruta
provisional del Goblin King. El pool VLT1 nivel `piso + 3` sigue siendo una
aproximación: el tipo/RNG de la recompensa nativa no está recuperado. La IA,
acciones, fuego, colisión ofensiva y transición de campaña del Dragon siguen
pendientes; derrota/recompensa no equivalen a un jefe completo.

Validación local: 198 pruebas Python y 12 pruebas C host pasan. Las regresiones
C cubren derrotar al Dragon mediante melee y flecha y la creación de una sola
caja. Se regeneraron proofs de ruta de los pisos 0/1/2; la campaña valida 142
archivos y el replay tiene 250 pasos / 2,206 ticks nominales.

El VPK 01.35 se compiló con VitaSDK en Windows porque WSL devolvió
`Wsl/EnumerateDistros/Service/E_ACCESSDENIED`. Artefacto:
`_local/build-act-one-135/Vagante2Vita-iteration135.vpk`; tiene 7 entradas,
`eboot.bin` de 191,586 bytes y SFO 01.35. SHA-256:
`F9BE938916893C4F224ED492672F1C1406866C6A442CE3735567577837EE19AE`.

La revisión de la instalación local encontró que el harness anterior solo
copiaba `vs0` y `os0`; omitía `ux0`, `sa0`, cachés y preferencias. Se añadió
`-CopyCompleteInstance` a `tools/test_vita3k.ps1` para clonar el perfil entero
a un `TestRoot` nuevo, mantener el filesystem separado del runtime, y guardar
la ruta de runtime en `run.json` para que Collect lea los logs correctos. La
carpeta fuente no se modifica. La copia 0.2.1.4183 comprobó 2,018 archivos y
352,877,427 bytes con hashes coincidentes; se preparó además una copia del
perfil completo con el runtime 0.2.1.4190 (159 archivos de runtime).

Repeticiones con perfiles completos y el VPK 01.35 (SHA-256 arriba) siguen
deteniéndose antes de crear sesión nativa. Se probaron Vita3K 0.2.1.4183
(QT/Vulkan, ID instalado, 90 s; consola/ruta VPK, 90 s; SDL/Vulkan, 240 s y
SDL/Vulkan con cachés regeneradas, 45 s) y 0.2.1.4190 (SDL/OpenGL,
ID instalado, 180 s). También se clonó el perfil 4170 cuyo ejecutable coincide
por hash con un smoke histórico exitoso y se probó con SDL/Vulkan, ID
instalado, con límite de 240 s. Los procesos 4170 y 4183 seguían detenidos
antes del log nativo cuando se inspeccionaron y se cerraron manualmente antes
de alcanzar ese límite; consumían menos de 0.1 s de CPU. Los logs de Vita3K,
debug, stdout y stderr quedaron vacíos. El resultado del último control 4183
está en
`_local/vita3k-complete-test-135-4183-cache-reset/runs/iteration135/run-000001/`.
Las pruebas `tests/test_vita3k_harness.ps1` (3/3) y
`tests/test_vita3k_smoke.py` (11/11) pasan, pero solo verifican clasificación
de fallos y análisis de logs, no la integración gráfica. Copiar todos los
datos no resolvió el bloqueo; la integración del VPK en Vita3K sigue sin
verificarse. Para repetir desde el perfil local completo:

```powershell
tools/test_vita3k.ps1 -Action Run -Iteration 135 `
  -EmulatorDirectory _local/vita3k `
  -TestRoot _local/vita3k-complete-test-135 `
  -CopyCompleteInstance -Vpk _local/build-act-one-135/Vagante2Vita-iteration135.vpk `
  -ResourceDelivery _local/analysis133/campaign-delivery `
  -ActOneCampaignSmoke -Frontend sdl -Renderer OpenGL -TimeoutSeconds 180
```
