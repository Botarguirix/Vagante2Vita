# Iteración 045: estado propio para BoulderTrap

La entidad BoulderTrap deja de reutilizar los campos del proyectil DartTrap
para representar su placa. `WorldEntity` ahora conserva explícitamente si la
placa está abajo, si se ejecutó la inicialización y si ya registró su primera
activación. La inicialización comienza con la placa presionada y cuenta 60
ticks antes de levantarla; el primer solapamiento posterior la presiona y
registra el evento una sola vez. Esto corrige el estado implícito de 044 y
mantiene aislado el estado del proyectil de dardos.

La mecánica aún no lanza ni simula Boulder, no aplica daño y la comprobación de
solapamiento sigue siendo una aproximación al despacho de colisiones original.
El temporizador de 60 ticks está confirmado por `BoulderTrap::step` en la
decompilación local. El runtime todavía debe recuperar el momento y los
parámetros exactos de creación del Boulder antes de implementar esa parte.

## Verificación

- VPK `00.45` compilado con VitaSDK Windows; Title ID `VAGV00006`.
- Python `unittest`: 119 pruebas OK.
- Compilación C de sintaxis estricta Vita (`-Wall -Wextra -Werror`) para los
  módulos y caso de entidad: correcta.
- La prueba C añade el retardo exacto de 60 ticks, la activación única y el
  estado presionado tras la activación. También verifica el caso de permanecer
  sobre la placa durante el armado: debe abandonarse y volver a pisarse antes
  de activar. El callback comprueba `prevDown` en el original. La prueba C no
  se ejecutó nativamente porque no hay compilador host configurado.
- Vita3K no validado: las ejecuciones anteriores de la instancia aislada
  expiraron antes de crear una sesión nativa; este VPK aún no se ha probado allí.
