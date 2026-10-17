# Iteración 085: replay de Vita3K con pasos fijos

La ejecución manual de 00.84 cargó ACT_ONE, pero su registro se detuvo antes
de las transiciones. Las rutas se habían grabado con una entrada por tick de
física; el bucle principal podía procesar varios ticks atrasados con el mismo
estado de mando y avanzar el replay después, desplazando los bordes de entrada.

La versión 00.85 limita la recuperación de tiempo a un tick por cuadro mientras
el replay está activo. Así cada tick simulado consume el estado de mando que le
corresponde, incluso si Vita3K baja de velocidad. `replay_advance` también
consume correctamente más de un segmento si recibe varios ticks. La prueba C
comprueba tanto los bordes entre segmentos como el avance acumulado. Pasan 159
pruebas Python y 12 tests C del runtime, compilados y ejecutados en WSL.

VitaSDK generó `_local/build-act-one-085/Vagante2Vita-iteration085.vpk`
(830,419 bytes, SHA-256
`B7130B6B6D20081A4B93B67EDEE47CAC42D64A47DAE5FF6AB7982A536433D396`). El VPK
se instaló en el perfil aislado
`_local/vita3k-vagante-clone/filesystem/ux0/app/VAGV00006`; la versión 00.84
quedó respaldada en `runs/iteration085-app084-backup`. El replay de los tres
pisos está preparado en `ux0:data/Vagante2Vita/autotest.vts`. La validación
nativa 00.85 está pendiente de iniciar el título y revisar las transiciones,
el cierre del tercer piso y el consumo del replay.

Esta iteración no declara ACT_ONE completo: generación exacta por semilla,
paridad integral de enemigos e inventario, jefe y validación en hardware Vita
siguen pendientes.

Las pruebas de rutas se regeneraron en WSL para los tres pisos con el simulador
actualizado; la extensión privada de campaña valida sus 100 archivos. Al volver
a ejecutar el arnés con esos proofs y este VPK, Vita3K terminó con
`0xC0000005` antes de crear un registro nativo. El smoke no pudo comprobar el
consumo del replay ni las transiciones. El fallo queda registrado; no se declara
completo ACT_ONE.

## Reintento en el perfil dedicado

El paquete privado `delivery-act-one-campaign085` se corrigió a 105 archivos:
además del VGA1 de ataque Goblin, incorpora las poses `attack.png` de los cuatro
tipos de enemigo que exige el cargador visual. El manifiesto vuelve a validar y
el replay de campaña se generó con 545 pasos para las rutas de los tres pisos.
Los recursos corregidos quedaron copiados en
`_local/vita3k-vagante-dedicated/filesystem/ux0/data/Vagante2Vita/`.

El arnés probó lanzamientos Qt y SDL con OpenGL, y el modo de consola con VPK
directo. Los tres procesos quedaron sin ventana principal, sin salida y con
`vita3k.log` vacío. SDL y consola expiraron a los 45 segundos antes de crear
`session-*.log` (`run-000007` y `run-000008`, etapa `before_native_session`).
Por tanto, estos intentos no evalúan los recursos corregidos ni el recorrido de
ACT_ONE. Los logs anteriores sí documentan los dos bloqueos de recursos ya
corregidos en el perfil: primero faltaba `act-one-goblin-attack.vga`, luego
`enemies/goblin/attack.png`. Se requiere un arranque interactivo funcional de
Vita3K para validar el replay actual y las transiciones; no se declara el mundo
completo ni jugable todavía.
