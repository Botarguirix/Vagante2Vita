# Iteración 01.44: decisión de salida de campaña en runtime

La decisión al tocar una salida quedó extraída al módulo portable
`vita/src/campaign_flow.c`, que usa directamente `vita/src/main.c`. El helper
devuelve una de cuatro rutas: avanzar desde los pisos 0/1, bloquear la salida
final si no hay jefe cargado o queda alguno vivo, completar el acto cuando los
jefes cargados fueron derrotados, o reiniciar la ruta intro cuando el índice
de campaña es `-1`. Muerte o ausencia de contacto con la salida no provocan
transición. La tabla de casos está en `tests/campaign_flow_test.c`.

Se reconstruyeron los proofs y sidecars de los tres pisos con la huella actual
del simulador (`3bc8a01001ac974694e784373683ec4639820b1134096ddc85b8e6e564eb343b`).
Las tres rutas host pasan; el piso final derrota el Goblin King y sale con 41
HP. La entrega regenerada está en `_local/analysis144/campaign-delivery/`, con
142 archivos validados y replay de 494 pasos / 3,821 ticks. Como la geometría
de pisos 1 y 2 y el combate/recompensa del jefe siguen siendo provisionales,
`campaign_complete=false` se conserva.

El VPK VitaSDK 01.44 está en
`_local/build-act-one-144/Vagante2Vita-iteration144.vpk`. Su identidad es
`VAGV00006` / `Vagante2Vita` / `01.44`, contiene siete entradas permitidas, y
su `eboot.bin` es el mismo producido por este build. SHA-256 del VPK:
`87C28A12A053DF53E75398B2FCFB9E8AC2CB0FA5405C6F48001D2E14CAA96FCB`. El
símbolo `campaign_exit_action` está enlazado en el ejecutable ARM.

Validación host: 13 pruebas C pasan (incluida la tabla nueva); 202 pruebas
Python pasan. La prueba automatizada de Vita3K instaló el VPK 01.44 en un clon
completo aislado, validó el bundle y preparó el replay, pero Vita3K agotó 30 s
sin crear log de sesión nativa; stdout, stderr y `vita3k.log` quedaron vacíos.
Registro:
`_local/vita3k-vpk144-test2/runs/iteration144/run-000001/`. El problema ocurre
antes del runtime del juego y este resultado no verifica su comportamiento.

WSL sigue denegado, por eso el build se realizó con VitaSDK Windows. Dragon
sigue sin IA/contacto nativo y Worm requiere su representación enlazada de 20
segmentos. La generación exacta, la recompensa nativa, el replay real en
Vita3K y hardware Vita continúan pendientes.

Seguimiento de diagnóstico: se comparó la réplica completa contra su origen.
Ambas contienen los mismos 1,737 archivos del filesystem, 162 del runtime,
13 de cache, 14 plugins, 13 archivos de `share`, 22 shaders integrados y 53
traducciones; también están presentes `ux0/app/VAGV00006` y los datos del
título. La prueba se repitió en una copia completa con frontend Qt (45 s) y
consola/VPK directo (45 s); los dos procesos expiraron antes del log de sesión,
con logs de Vita3K vacíos. Esto descarta una omisión obvia de archivos y una
diferencia entre instalación por ID y ruta VPK; sigue sin identificar por qué
el bootstrap del emulador no responde. Evidencias adicionales en
`_local/vita3k-vpk144-qt-test/runs/iteration144/run-000001/` y
`_local/vita3k-vpk144-console-test/runs/iteration144/run-000001/`.

La prueba más reciente valida SHA-256 de todos los archivos del clon antes de
editarlo: `COMPLETE_INSTANCE_COPY verified=1 files=2126`. En la verificación
independiente, los 2,124 archivos restantes coincidieron con el origen; solo
cambiaron `runtime/config.yml` y el paquete VAGV00006 instalado por la prueba
(`eboot.bin` y `param.sfo`). Aun así, SDL/OpenGL volvió a agotar 30 s antes de
la sesión nativa. Un arranque Qt manual en ese mismo clon permaneció vivo pero
sin ventana, sin log y con CPU estable en 0.078 s durante diez segundos. El
proceso aislado se cerró al terminar la observación. El runner ahora rechaza
cualquier omisión, archivo extra o hash distinto durante la clonación completa.
