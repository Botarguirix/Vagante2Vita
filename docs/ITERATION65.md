# Iteración 065: estado real de comportamiento de entidades

El log del runtime ya no marca como pendiente cualquier paquete VWE2. Ahora
cuenta solo `BreakableWall` y `Boulder` suelto (kinds 21/22), cuyo
comportamiento sigue sin estar integrado. Los cofres, pinchos, DartTrap,
doodads y BoulderTrap que aparecen en los tres pisos del bundle tienen sus
manejadores actuales y producen `behavior_pending=0`. El exportador describe
ese alcance y el smoke ACT_ONE exige ese valor para no aceptar pisos con
entidades sin soporte.

Los paquetes VWE2 de los pisos 0, 1 y 2 contienen respectivamente 44, 29 y 45
entidades; sus tipos son 16–20 y 23. Por tanto, no contienen BreakableWall ni
Boulder independiente. Los enemigos siguen siendo provisionales y su paridad
con la generación del juego original sigue pendiente.

La suite Python completa pasa 137 pruebas. El VPK
`_local/build-act-one-065/Vagante2Vita-iteration065.vpk` compiló con VitaSDK,
abre como contenedor con las siete entradas previstas, tiene 824837 bytes y
SHA-256 `D76827D04AC45B8075509BC870DBEEAD03910558412AB72E659FAE67795613A1`.
El test C que comprueba el nuevo conteo de log no se ejecutó porque no hay
toolchain host/CMake disponible; el módulo sí compiló para Vita.

La validación Vita3K aún falla antes de abrir la interfaz o crear un log. Se
probaron SDL/Vulkan con instalación aislada, consola/OpenGL con VPK directo,
Qt visible y una sonda `QT_DEBUG_PLUGINS`; los procesos no crearon ventana,
stdout/stderr ni log del emulador. Se cerró únicamente una instancia de
prueba vieja de VaganteVita (`_local/vita3k-act-one-062`); las instancias
pertenecientes al otro proyecto permanecieron intactas. El primer mundo sigue
incompleto por la paridad RNG/endpoint, enemigos provisionales y falta de un
smoke real de transición en Vita3K.
