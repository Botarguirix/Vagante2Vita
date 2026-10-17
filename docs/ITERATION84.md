# Iteración 084: panel seguro para objetos recogidos

Los objetos que caen de cofres entran al inventario con rol `START_MISC`. El
panel de personaje solo tenía etiquetas para MELEE, RANGED y AMMO y usaba el rol
como índice directo, de modo que abrir el panel después de recoger botín podía
leer fuera del arreglo. La versión 00.84 agrega la etiqueta MISC, valida el
índice, limita el render a seis filas y muestra cuántos objetos adicionales
quedan en el inventario.

El VPK `_local/build-act-one-084/Vagante2Vita-iteration084.vpk` compiló con
VitaSDK para Windows. Mide 830,266 bytes; SHA-256:
`5B047B82A035DCFF0C8832B9BF7159529E34236EE1223A91D54A1CE327ED60E2`. El
`native_ui.c` modificado compiló individualmente con `-Wall -Wextra -Werror`.
La suite Python completa pasó: 159 pruebas.

El perfil dedicado de Vita3K ya abre correctamente desde el escritorio y su
biblioteca muestra `VAGV00006`, versión `0.84`, desde
`_local/vita3k-vagante-clone/filesystem`. Para instalarla se respaldó la carpeta
0.83 en `_local/vita3k-vagante-clone/runs/iteration084-app083-backup` y se
actualizó el directorio del título con los siete archivos del VPK. La replay de
campaña (545 segmentos, 3,184 ticks, trazas `player_step:0,1,2`) está preparada
en `filesystem/ux0/data/Vagante2Vita/autotest.vts` y en
`_local/vita3k-vagante-clone/runs/iteration084/manual-run-000002/input.vts`.

La prueba manual desde la biblioteca sí inició el VPK 0.84. El log nativo
confirma `BOOT version=00.84 iteration=084`, carga del archivo del mundo y
`AUTOTEST enabled=1 steps=545 scheduled_ticks=3184`. El juego terminó limpiamente
con `EXIT normal=1 screen=WORLD display_error=0`, pero el último estado registrado
fue el tick 3012 y solo hubo un `WORLD_START`; no aparecen cambios de piso ni el
marcador de replay consumido. El checker de campaña falla los requisitos de pisos
1–2, transiciones, limpieza de mazmorra y consumo del replay. Así que el VPK
arranca y carga el primer piso, pero esta ejecución no valida todavía el recorrido
completo de ACT_ONE. El replay se volvió a dejar en
`filesystem/ux0/data/Vagante2Vita/autotest.vts` para repetir la prueba. La
generación exacta por semilla, inventario fiel al original, enemigos de pisos
posteriores y jefe también siguen pendientes.
