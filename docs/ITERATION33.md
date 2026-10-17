# Iteración 033: primer recorrido físico del piso base

El probe ACT_ONE ahora elige controles mediante predicción corta sobre copias de
`NativePlayer` y `World`, ejecutando la misma función `player_step` que usa el
juego. Esto reemplaza la heurística de perseguir tiles que podía insistir en
subir por aire o competir con la escalada.

Con seed 12345, el mapa base de 80×40 alcanza la salida en 656 ticks con 10
saltos. Resultado observado en WSL/GCC host: `reached_exit=1`, waypoint 58/59,
jugador en (590.97, 445.76). Es evidencia de recorrido del piso base en la
física actual del runtime; no demuestra paridad de generación ni transición
entre pisos.

El piso 1 aún no pasa: con su ruta ponderada, la simulación agota 180.000 ticks
en el waypoint 17. El piso 2 también agota 180.000 ticks en el waypoint 26.
Una variante que solo admitía subida por escalera informó que el piso 1 no tiene
una ruta de esa clase. Los caminos tile-only aún incluyen ascensos por aire y
el controlador predictivo no resuelve de forma robusta esos saltos; el siguiente
trabajo es representar saltos alcanzables como aristas del planificador y
verificar entrada, salida y transición en los tres pisos.

Los 11 tests nativos pasan en WSL. El VPK vigente sigue siendo 00.32; esta
iteración cambia la herramienta host de validación y documentación, no acredita
un VPK ACT_ONE completo. Continúan pendientes Vita3K con el paquete actualizado,
paridad exacta, entidades y transición final.
