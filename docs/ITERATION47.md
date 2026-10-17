# Iteración 047: temporizador de liberación de BoulderTrap

El estado portable de BoulderTrap conserva el armado inicial de 60 frames y
ahora modela la secuencia de `triggerFrames`: después de la primera activación
válida, el estado portable marca la señal de soltar el Boulder al sexto paso. La placa
sigue requiriendo reentrada después de levantarse y solo puede activar una vez.
El contador está respaldado por `BoulderTrap::step` (`triggerFrames < 6`, luego
`Boulder::release`) y el retardo inicial de `resetDelay=0x3c`.

Esto solo cubre el reloj de la trampa. No se despacha aún un actor Boulder en el
runtime, así que el marcador no lo hace aparecer ni moverse. Faltan su integración,
colisión con terreno/unidades y el daño original. Por tanto, la iteración
no declara BoulderTrap jugable ni el primer mundo completo.

## Verificación

- VPK `00.47` compilado con VitaSDK Windows; Title ID `VAGV00006`.
- SHA-256: `4a11120469b6406f6b8d85cbdc7d75e371eab3f821ba071c6558b7e47c5f9e8c`.
- La fuente de regresión C cubre armado inicial, reentrada requerida y marca la
  señal al sexto paso posterior a la activación. Compiló con banderas estrictas
  ARM, pero no se pudo ejecutar como test C host.
- `unittest`: 119 pruebas OK. Compilación C estricta Vita correcta.
- WSL devuelve `E_ACCESSDENIED` al enumerar distribuciones; la prueba C no se
  pudo ejecutar en Linux en esta máquina.
- Vita3K no validado: el intento de abrir la instancia aislada quedó sin ventana
  ni log de sesión nativa, así que no demuestra arranque del juego.
