# Iteración 089: geometría de aparición del Lurker

La rama ACT_ONE de `LevelGenerator::addRandomEnemies` descarta un roll de
Lurker si hay suelo sólido en `(x, y-1)` o si los dos vecinos horizontales
`(x-1, y)` y `(x+1, y)` son sólidos. `Level::isSolid` devuelve el estado del
tile para las coordenadas dentro del mapa. El modelo Python incorpora esa
condición y continúa con otros candidatos tras un descarte, hasta alcanzar el
peso permitido por sala o quedarse sin celdas.

El bundle privado `_local/delivery-act-one-campaign089` valida 108 archivos y
56 imágenes. Sus pisos tienen 8, 8 y 20 spawns provisionales. En la sala
`[3,0]` del piso 2, el modelo examinó tres celdas para aceptar dos enemigos,
confirmando que el filtro rechazó un candidato Lurker. Los VWI2/VWE2 siguen
coincidiendo con los tres proofs físicos de ruta seed 12345; solo cambió VWE3
del piso 2. Pasan 166 tests Python.

Vita3K 00.86/000003 volvió a agotar 90 segundos antes de una sesión nativa.
El binario 0.2.1.4183 sí abre la interfaz cuando se inicia mediante el control
de aplicaciones de Windows; su VitaFS dedicado muestra Vagante 00.86 instalado
y el log confirma que cargó el perfil `VitaFS` del proyecto. La interfaz se
quedó en un diálogo de actualización cuya automatización no pudo accionar. Se
desactivó `check-for-updates-mode` en el config del clon dedicado. El VPK 00.89
se compiló con VitaSDK Windows: 830,426 bytes, SHA-256
`008900C3E17DC813018E8984065479B883A531FE6823AA5E7F859EF8C9C8D3D2`. Su
preflight aislado pasó con el bundle 089 (108 archivos, 56 imágenes, 3 pisos)
y preparó una replay de 545 segmentos/3,184 ticks ligada a rutas `player_step`
de los tres pisos. Vita3K 0.2.1.4183 agotó 90 segundos sin crear ventana de
juego, log del emulador ni sesión nativa; el resultado quedó en
`_local/vita3k-vagante-test089-build/runs/iteration089/run-000001/result.json`.
También se comprobó que lanzar `--help` por el mismo canal de procesos queda
abierto y mudo, lo que sitúa el bloqueo antes de ejecutar el VPK. El perfil
dedicado en modo GUI sí abrió en la sesión de escritorio previa, pero la
automatización de escritorio no está conectada en este turno.

Los filtros de candidato, barajado y RNG aún no tienen paridad total; solo se
recuperó esta regla de vecinos para Lurker. El jefe, las transiciones y la
ejecución de campaña en Vita3K/hardware siguen pendientes. `floor_complete` y
`campaign_complete` permanecen falsos.
