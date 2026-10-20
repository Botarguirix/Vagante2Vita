# Iteración 01.59: entrada desde la cueva y quitar el reinicio con Círculo

El reporte y la captura identificaron dos rutas que podían parecer el mismo
problema. La pantalla de juego mostraba `O RESET`, y Círculo reaparecía en el
spawn cuando se pulsaba. Se quitó esa asignación de depuración para que Círculo
no reinicie el mundo. Además, si el juego carga el mundo inicial de respaldo
(índice -1) y el jugador toca su salida/cueva, el runtime intenta cargar el
piso 0 de ACT_ONE. Si sus datos no están disponibles, muestra el bloqueo con
un log explícito, en vez de reaparecer silenciosamente en el intro.

La indicación de entrada ahora muestra `D-PAD/STICK: RIGHT THEN DOWN+X TO
DROP`, que corresponde al prefijo de la ruta validada desde el spawn: avanzar
a la derecha hasta la plataforma y usar abajo+X para dejarse caer. El primer
tramo del mapa sí tiene una pared al avanzar demasiado a la derecha: el probe
con el VWI de piso 0 coloca al jugador en x=532, lo detiene contra el muro en
x=599.999 al tick 48 sin invertir su posición. Es contacto sólido normal; el
descenso del primer tramo requiere abajo+X.

Verificación: pasan 13 grupos C host, 204 pruebas Python y las rutas host de
los tres pisos (424, 897 y 5,467 ticks; Goblin King derrotado, jugador con 87
HP). El paquete privado de campaña valida 142 archivos y la replay de tres
pisos tiene 952 pasos / 7,120 ticks nominales.

VPK compilado con VitaSDK: `_local/build-act-one-159/Vagante2Vita-iteration159.vpk`
(843,098 bytes, SHA-256
`04C860605B8619E52B33F2CAE5DB9DB1414BEC6A47A7135D920A7B3D0776981A`). La
prueba Vita3K 0.2.1.4190 se intentó en un clon completo aislado de 2,162
archivos, tanto Vulkan/Qt como OpenGL/SDL y modo consola/OpenGL con instalación
del VPK por CLI. También se intentó 0.2.1.4170 OpenGL/SDL en otro clon privado
verificado de 1,669 archivos. Todos agotaron 120 s o terminaron con
`0xC0000005` antes de crear sesión nativa (`before_native_session`); ninguno
ejecutó el juego ni gameplay. El Vita3K compartido de Touhou no fue modificado.

Por tanto, la solución está cubierta por lógica, rutas y pruebas host, pero
todavía no se ha confirmado visualmente que la puerta del usuario transicione
al primer piso en un emulador que llegue a ejecutar la app.

Reintento de diagnóstico (18-10-2026): se usó como origen el perfil local
`_local/vita3k`, que conserva una corrida 01.25 previamente aprobada. El
harness creó y verificó un clon completo separado de 2,018 archivos, instaló
el VPK 01.59, validó el paquete de campaña de 142 archivos y preparó la replay
de 952 pasos / 7,120 ticks. En el clon, Vita3K quedó vivo pero no creó sesión
nativa ni `vita3k.log` durante 150 s; resultado `before_native_session` en
`_local/vita3k-act-one-159-verifiedbase/runs/iteration159/run-000001/result.json`.
El proceso de prueba se detuvo al expirar el plazo. El perfil de origen y la
instancia compartida permanecieron intactos. Este intento tampoco constituye
una prueba de gameplay.
