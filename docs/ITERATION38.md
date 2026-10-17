# Iteración 038: DartTrap con proyectil y daño

Ghidra había confirmado que `DartTrap::doCollision` dispara solo cuando un
actor elegible entra en el corredor situado delante de la trampa; no dispara
al cargar el piso. El runtime ahora activa DartTrap cuando el jugador está
hasta 96 px en la dirección de disparo y a la altura del cuerpo de la trampa.
Esta distancia/altura es una aproximación: la función recuperada muestrea una
región de colisión, pero la pseudodecompilación tiene temporales y punteros
optimizados que impiden fijar con confianza sus bordes exactos.

`world_dart_traps_step` crea un ArrowTrap horizontal a ±5 px/tick, bloquea el
dardo con tiles sólidos, comprueba una caja provisional de 16×4 px contra el
jugador y aplica 5 de daño escalados por `damage_taken_multiplier`. Trampas de
un uso quedan disparadas; las recargables esperan 130 ticks tras acabar el
proyectil y pueden volver a activarse. Se dibuja el dardo usando
`entities/dart.png`, que debe estar en la entrega privada de recursos. El daño
escalado, filtro de actor, impacto en bordes/agua y temporización exacta de la
recarga aún requieren comparación con ejecución del juego original.

## Verificación

- Suite Python: 119/119.
- `arm-vita-eabi-gcc -Wall -Wextra -Werror -fsyntax-only` pasó sobre
  `world_combat.c`, `player.c`, `world_visuals.c` y `player_test.c`.
- VPK compilado con VitaSDK Windows: `VAGV00006`, `00.38`, 808297 bytes,
  SHA-256 `E30ACB5A3AB6505F1F5E6E4661E74841860EBA52ACD1891977F5A36FE8408B40`.
- WSL volvió a denegar inicio (`E_ACCESSDENIED`), por lo que no se ejecutó
  CTest host. El test C de jugador sí se compiló en modo estricto, no se ejecutó.
- Smoke aislado ACT_ONE en Vita3K: falló con timeout de 60 s; el proceso terminó
  con código -1, stdout/stderr y `vita3k.log` vacíos, sin sesión nativa. El
  resultado está en `_local/vita3k-iteration038/runs/iteration038/run-000001/`.
  Repeticiones aisladas con OpenGL/SDL, Vulkan/SDL y Vulkan/Qt visible dieron
  el mismo síntoma; en Qt el proceso tampoco creó una ventana. La consulta
  `--help` al ejecutable clonado también quedó esperando 10 s, así que Vita3K
  parece bloquearse antes del frontend en estos clones. Esto no determina si
  el origen es configuración, entorno gráfico o runtime del emulador.

El piso ACT_ONE no se declara completo: generación original, loot, enemigos
fieles, transición de salida, pisos siguientes, jefe y validación Vita3K siguen
pendientes. Los recursos comerciales permanecen externos al VPK.
