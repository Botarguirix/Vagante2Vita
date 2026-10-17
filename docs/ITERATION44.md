# Iteración 044: ciclo visible de BoulderTrap

Se conectó el tipo VWE2 20 a los recursos y al tick del juego. La BoulderTrap
usa `entities/trap_trigger.png` en sus dos primeros frames de 16×16; empieza
presionada, se mantiene durante el retardo inicial de 60 ticks confirmado en
`BoulderTrap::step`, y después representa la placa arriba/abajo según el
contacto con el jugador. Solo registra el primer contacto que sucede cuando
la placa ya estaba levantada, siguiendo la condición de una sola activación de
`BoulderTrap::trigger`. El runtime escribe `WORLD_INTERACTION` para ese evento.

La detección de contacto usa intersección de rectángulos del jugador y la placa
como aproximación portable al despacho de colisiones por entidades del original.
El ciclo de liberación a los seis ticks está documentado en Ghidra, pero el
runtime aún no instancia ni simula Boulder; el contacto no hace daño ni dispara
una roca. La lógica exacta de `Boulder::step`, daño y colisión sigue pendiente.
El bloqueador de roca conserva su integración visual de la iteración 043.

## Verificación

- VitaSDK Windows compiló VPK `00.44`, Title ID `VAGV00006`, 809,189 bytes.
- El SFO del VPK confirma `APP_VER=00.44`; el ZIP contiene las siete entradas
  esperadas de instalación/LiveArea.
- SHA-256 del VPK: `C0566CFA181275DC415E1B19D2A0E78F5EBD385BB5A3FFEFC759864A4CF6EB67`.
- Compilación C Vita con `-Wall -Wextra -Werror` en los módulos y el nuevo caso
  C de entidad; la prueba añade checks de parser, retardo inicial, subida y
  activación única. No hay compilador C de escritorio disponible para ejecutar
  el binario de prueba host aquí.
- Python `unittest`: 119 pruebas OK. Campaña privada 049: manifiesto válido,
  68 archivos.
- Vita3K 0.2.1.4183 siguió agotando el timeout antes de crear un log de sesión
  (también para 00.44, OpenGL/SDL e instalación normal). Por ello no se atribuye fallo a la
  trampa ni se afirma que la transición haya sido validada en el emulador.
