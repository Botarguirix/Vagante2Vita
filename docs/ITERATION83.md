# Iteración 083: mantener activa la replay hasta la salida final

La versión 00.82 podía terminar la replay al cargar el tercer piso: el
indicador `act_one_exit_reached` conservaba el valor del piso anterior. La
iteración 083 reinicia ese indicador cuando carga cada nuevo piso. Así, el
marcador `act_one_campaign_sequence_consumed` solo puede aparecer después de
reproducir la ruta que toca la salida del piso 2.

El VPK `_local/build-act-one-083/Vagante2Vita-iteration083.vpk` se compiló con
VitaSDK instalado en Windows. Mide 830,165 bytes y su SHA-256 es
`D9DAFF77124974903BF1CC3DDEFDC6086FE241FFEBE3D94B8B231001955A2486`. El paquete
contiene los siete archivos esperados y el ejecutable ARM `eboot.bin` de 183,230
bytes. El bundle de campaña privado de 100 archivos pasó la validación de
manifiesto. La suite Python completa pasó: 159 pruebas.

La prueba de campaña para Vita3K preparó la replay de tres pisos (545 segmentos,
3,184 ticks) y lanzó Vita3K 0.2.1.4183 con su perfil clonado en
`_local/vita3k-vagante-clone`. El proceso Qt permaneció activo, pero Windows no
le asignó una ventana principal y no apareció ni el log del emulador ni una
sesión nativa. El controlador de escritorio de esta sesión no enumera ventanas
ni aplicaciones, así que no fue posible iniciar el título desde la biblioteca.
Se repitió fuera del arnés: el proceso siguió sin ventana principal y sin
diagnóstico estándar. También se probó el modo de consola con el VPK como
argumento directo; el resultado fue igual. Ambos procesos aislados se cerraron
por sus PID verificados, sin afectar otras instancias. El primer diagnóstico
está en `_local/vita3k-vagante-clone/runs/iteration083/run-000001/result.json`;
el segundo run conserva la evidencia en
`_local/vita3k-vagante-clone/runs/iteration083/run-000002/`. Por ello 00.83
compila y pasa las pruebas de herramientas, pero su ejecución nativa y el
marcador de salida final aún no están confirmados.

El build solicitado mediante WSL no se pudo hacer porque la distribución no
tiene VitaSDK (`VITASDK_NOT_FOUND`). Se usó el VitaSDK de Windows disponible en
el workspace. Para completar el smoke se necesita una instancia de Vita3K que
abra una ventana interactiva en este escritorio, o reparar la invocación Qt del
clon para que abra su ventana y permita ejecutar `VAGV00006`.

Las trazas de ruta y los pisos siguen siendo una reconstrucción. La campaña
actual no demuestra paridad de generación por semilla, enemigos completos ni
el jefe del acto.
