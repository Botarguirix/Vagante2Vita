# Iteración 01.67 — navegación de caída y combate final del Worm

Se corrigió el ciclo de navegación en floor 1: la etapa intermedia de caída
permanece activa hasta aterrizar en la plataforma inferior. Después del
aterrizaje el controlador avanza lateralmente hacia el waypoint; el combate
del Worm mantiene al jugador en el waypoint del jefe, y al quedarse sin
flechas usa melee. No se desactivó el spit ni se redujo la vida del jefe.

Las pruebas físicas de la campaña ACT_ONE pasan en los tres pisos, con el
scheduler de spit activo. Floor 1, seed 49, derrota al Worm y alcanza la salida
en 1,650 ticks con 14 HP. Los proofs v3 y trazas VTR están en
`_local/routes-worm-campaign-melee-finish-20261018/`; el manifiesto de
resultados confirma `passed: true` para los tres pisos. Las 13 suites host
también pasan en `_local/host-tests-iteration167/report.json`.

El VPK local 01.67 se compila con VitaSDK Windows. WSL devolvió
`Wsl/Service/CreateInstance/E_ACCESSDENIED`. La prueba anterior con Vita3K
desde `C:\Vita3k` venció antes de crear la sesión nativa, por lo que esta
iteración tiene validación host completa pero no validación de gameplay en
Vita3K ni en hardware.

## Prueba Vita3K desde `C:\Vita3k`

Se reintentó usando el perfil ligero aislado del script. El bundle validó sus 144 archivos y el replay de 52 pasos se preparó, pero Vita3K venció a los 45 segundos antes de crear una sesión nativa; `stdout.log`, `stderr.log` y `vita3k.log` quedaron vacíos. El resultado está en `_local/vita3k-cdrive-light-profile-167/runs/iteration167/run-000002/`. La instalación y perfil compartidos no se modificaron. Por ello esta iteración tiene validación host completa, pero no validación de gameplay en Vita3K ni en hardware.

SHA-256 del VPK: `B0B44D4FBE91767D5A08C3E240580783056CD64582016A147C6FC0CAF1495E01`.

### Seguimiento de diagnóstico de inicio

Se repitió el replay con el perfil aislado existente y la instalación fuente en `C:\Vita3k`, variando la ruta de inicio y el renderer. En las tres ejecuciones Vita3K siguió vivo hasta el límite y no produjo log del emulador ni sesión nativa:

- `run-000002`: modo consola con VPK como argumento, OpenGL, 45 s.
- `run-000003`: interfaz Qt, título instalado por ID, OpenGL, 60 s.
- `run-000004`: interfaz Qt, título instalado por ID, Vulkan, 60 s.

Los detalles y resultados JSON están bajo `_local/vita3k-cdrive-light-profile-167/runs/iteration167/`. Cambiar de CLI a GUI y de OpenGL a Vulkan no alteró el resultado; las próximas pruebas deben centrarse en el arranque del perfil/runtime Vita3K. No se intervino en los procesos ni en el perfil fuente.
