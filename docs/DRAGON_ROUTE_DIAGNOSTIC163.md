# Diagnóstico de ruta del Dragon — iteración 01.63

El encuentro del Dragon se aisló en `_local/campaign-floor0-dragon-test163`; no
se añadió al bundle de entrega. El mapa ofrece seis anclas geométricas para su
hitbox 13×13. El ancla usada anteriormente `[21,25]` no permite una ruta host
estable: el arquero muere o las flechas chocan con terreno.

## Seguimiento de IA host (2026-10-18)

Los exports de Ghidra para `BossDragon::step` y `addNewActions` confirman IDs de
acciones y sus secuencias, pero no el estado RNG completo. El runtime ahora
prueba un ciclo determinista parcial de IDLE, MOVE_PLAYER, FACE_PLAYER y
SHOOT_PLAYER_ONCE. La rama de tiro conserva el gate de visibilidad y línea de
visión. Se dispara un solo proyectil: los argumentos `3/1` vistos en Ghidra son
selectores de estado y no se consideran cantidades de proyectiles. El
movimiento aún interpola directamente hacia el jugador y los 55 ticks de
preparación son aproximados; buceo, carga, RNG y animación nativa siguen sin
implementarse.

El barrido del probe con las seis anclas dio un caso repetible en `[20,29]`.
Tras derrotar al Dragon, el piloto detectaba que la ruta de salida pasaba bajo
un bloque sólido (`22,27`) y se atascaba intentando bajar. Se añadió un stage
de navegación post-jefe que busca un paso vertical libre con soporte y desplaza
el waypoint horizontalmente antes de bajar. Con ese ajuste, el Dragon cae tras
siete impactos y el arquero conserva 30 HP; alcanza la salida en 2,283 ticks y
28 saltos (`reached_exit=1`, `dead=0`). Las otras cinco anclas no completan la
pelea. El ancla `[20,29]` se usa ahora como preferencia del fixture de prueba;
no implica paridad con la selección RNG nativa.

El runner completo de rutas host pasó los tres pisos sobre el paquete fixture
con el Dragon en el piso 0: Dragon derrotado y salida en el piso 0 (2,283
ticks), salida del piso 1 (897 ticks) y Goblin King derrotado con salida del
piso 2 (5,467 ticks). Resultado:
`_local/dragon-campaign-routes163-final/route-results.json`. Esto prueba la
repetibilidad del simulador host para esta configuración de prueba, no el
paquete de entrega ni el runtime Vita.

La validación host actual pasa 206 pruebas Python y 13 grupos C. VitaSDK produjo
el VPK 01.63 de 843,322 bytes, SHA-256
`1B0E48E2E9DBE7ECBA6EE0C3892B98C27FFB32CEF5CA55EEB8F94D463A6DFCFD`; el verificador
VPK reporta siete entradas y ningún error o aviso. Esto verifica el paquete y
la compilación, no el runtime del Dragon.

Para Vita3K se regeneraron los tres route proofs con el fingerprint actual y se
ejecutó el smoke intro→piso 0 desde una copia aislada basada en `C:\Vita3k`,
usando el fixture del Dragon en `[20,29]`. Vita3K 0.2.1.4194 (SHA-256
`DFC5C7CB…B30C54FE`) permaneció activo durante el límite de 60 segundos, pero
no creó `vita3k.log`, logs de depuración ni una sesión nativa. El resultado del
harness es
`_local/vita3k-dragon-anchor20-29-163/runs/iteration163/run-000001/result.json`,
con `failure_stage=before_native_session` y `process_timed_out=true`. No es un
resultado de ejecución del juego.

Siguiente trabajo: corregir/recuperar el arranque de Vita3K para obtener una
sesión nativa y validar el flujo con replay. El resultado host ya cubre combate
y salida en los tres pisos del fixture. La selección nativa de sala y ancla,
las acciones/RNG completos de Dragon, Worm y la máscara final de jefes siguen
pendientes, por lo que ACT_ONE aún no está completo.
