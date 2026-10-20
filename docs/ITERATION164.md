# Iteración 01.64: flechas contra Worm y auditoría de rutas

El runtime ahora trata al Worm como jefe al resolver impactos de flecha. Las
flechas pueden impactar la cabeza o una de las 20 hitboxes segmentarias
provisionales; derrotarlo por flecha activa la máscara de jefes y el cofre de
recompensa. Un test C ejercita tanto el impacto en cabeza como el impacto en la
cola y comprueba la derrota. Las hitboxes de los segmentos y el comportamiento
de daño siguen siendo hipótesis del modelo host; no se presentan como paridad
con el juego original.

La prueba de campaña reveló que la ruta BFS de tiles no basta para validar un
encuentro. Se intentaron 22 ubicaciones provisionales y luego anclas dentro de
la sala seleccionada por el generador; el jugador puede completar el piso sin
jefe, pero el probe dirigido al Worm se atasca o muere. Con el candidato
`[27,13]`, la pelea no progresa más allá de 7.2 puntos de daño al jefe, y el
jugador queda separado por una ruta de colisión que el grafo de tiles no
representa. No se generó proof de derrota/supervivencia para el piso 1. Los
resultados están en
`_local/routes-worm-world164l/route-results.json` y su log de piso 1.

El probe de Worm reutiliza la ruta spawn→salida del control sin jefe y solo
activa combate a distancia al aproximarse físicamente al waypoint. Se amplió
el umbral de 96 a 128 px para probar el ancla `[38,14]`, sin cambiar el runtime
del juego. Con el ancla original `[27,13]`, Dragon y Goblin King completan sus
rutas, pero el jugador muere por enemigos en tick 1,119 con el Worm aún a
63.5/80 HP (`_local/routes-worm-world164s/route-results.json`). En la copia
diagnóstica del paquete VWE3 con el Worm movido a `[38,14]`, se obtienen tres
impactos y el jefe baja a 44.8/80 HP, pero el jugador muere a tick 1,004 por
enemigos normales. El control melee cercano del probe no evita esa muerte.
Esa ancla y su registro están en
`_local/worm-anchor-combat-sweep164/worm-anchor-38-14.vwe` y el log
`_local/worm-anchor-combat-sweep164/anchor-38-14-128.csv`. No se generó proof
de derrota/supervivencia del piso 1. También existe el barrido de anclas en
`_local/worm-anchor-combat-sweep164/results.json`.

Una táctica diagnóstica que pulsó `PLAYER_ATTACK` al tener un enemigo cerca
de espada tampoco produjo bajas: el jugador quedó con 20 HP, el Worm con
54.5/80 a tick 1,200, y murió a tick 1,434. Resultado:
`_local/routes-worm-world164x/floor-01.log`. La medición no representa un
controlador eficaz y se retiró del probe. El VGA1 `goblin-stab` se aplica del
goblin al jugador dentro de `world_enemies_contact_damage`; no es el guion de
ataque del jugador contra enemigos. La espada del jugador usa su propio
`player-stab`, y no se confirmó aquí que esta lógica golpee goblins en juego.

El umbral del controlador del Worm quedó en 128 px y su lógica de disparo
usa las hitboxes provisionales de cabeza/segmentos. Un experimento posterior
que adelantó el waypoint del jefe antes de reproducir el mapa estancó al
jugador en la ruta BFS (tick 7,345, Worm intacto); se retiró. No es una mejora
jugable y no forma parte del VPK. La evidencia de `addBoss` (`0x0068a770`)
confirma conversión de tiles candidatos a píxeles para los constructores, por
lo que el fallo no se corrige con un offset oculto: la selección de habitación,
los filtros/bloqueos y el estado RNG anteriores aún no son seed-exact. El
próximo trabajo debe resolver ese estado antes de proponer anclas de Worm.

Se añadió `--position-log` para registrar trayectorias físicas durante el
diagnóstico. El preflight que pasa y los resultados Dragon/Goblin no sustituyen
una prueba de campaña completa: la ruta física al Worm sigue sin resolverse.

La suite Python pasó 209 pruebas y los 13 grupos C host pasaron en la validación
final de esta ronda. El probe recompilado conserva Dragon/Goblin King, pero
una repetición del Worm murió en tick 1,250 con 72.8/80 HP; la ruta del piso
1 continúa sin proof y es sensible a su controlador provisional. Se compiló el
VPK 01.64 con VitaSDK y optimización `Os`; contiene 7 entradas, identifica
`VAGV00006` / `Vagante2Vita` / `01.64`, y mide 780,584 bytes. SHA-256:
`2653362616d2121b2003fd24f98405f9a4fc8fe6ef575301b2e35727e5eb89de`.
Artefacto: `_local/build-act-one-worm-arrows164/Vagante2Vita-iteration164.vpk`.

Vita3K no validó esta iteración. La repetición con el VPK 01.64 se hizo desde
`C:\Vita3k` (0.2.1.4194, SHA-256
`DFC5C7CBAF161993636B194E7249EB860CBCC7CC6C35D6AEF79EC7A2B30C54FE`) en una
copia completa e aislada: 141 archivos de runtime y 2,600 archivos del perfil
fueron verificados. El preflight de los 142 recursos de ACT_ONE pasó y la
replay de intro se preparó con 52 pasos / 1,321 ticks. OpenGL/Qt agotó 75
segundos antes de crear sesión nativa; `vita3k.log`, stdout y stderr quedaron
vacíos. Resultado:
`_local/vita3k-c164-intro-opengl-try4/runs/iteration164/run-000001/result.json`.
El hash del VPK fue
`2653362616D2121B2003FD24F98405F9A4FC8FE6EF575301B2E35727E5EB89DE`.
Como el título no llegó a ejecutarse, esto no valida ni refuta el cambio de
flechas de 01.64. El perfil original y el proceso de `C:\Vita3k` no se
modificaron.

Una repetición adicional probó modo consola con el VPK como argumento directo;
también agotó 60 segundos antes de una sesión. El depurador aislado lanzó
0.2.1.4194 con SDL/OpenGL y `--skip-title`; capturó una violación de acceso en
`ucrtbase.dll+0x25F31`, desde `Vita3K.exe+0x683965`, con marcos de Qt6Core en la
pila. Evidencia guardada en
`_local/vita3k-c164-4194-sdl-debug.txt`. Esto confirma un fallo del emulador
antes de cargar el juego y no constituye una prueba del VPK. El resultado del
intento en modo consola está en
`_local/vita3k-c164-console-try1/runs/iteration164/run-000001/result.json`.

## Pendiente

- Recuperar el estado de RNG y filtros de `addBoss` para producir anclas
  nativas de jefe, en lugar de candidatos deterministas provisionales.
- Rehacer la navegación de prueba con colisiones físicas y demostrar un
  encuentro de Worm alcanzable, derrota, supervivencia y salida del piso 1.
- Completar el disparo ácido y verificar daño, hitboxes, muerte/recompensa y
  transición de campaña contra Ghidra.
- Resolver el arranque pre-sesión de Vita3K y repetir la campaña 01.64 con un
  perfil aislado; aún no hay validación nativa ni paridad exacta de generación.
