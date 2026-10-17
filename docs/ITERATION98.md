# Iteración 098: salto de Spider y revalidación de campaña

El runtime ahora ejecuta el ciclo de salto de Spider: aviso de 20 ticks en la
rama cercana, impulso, gravedad y colisión con tiles hasta aterrizar, seguido
de 30 ticks de recuperación. Los rangos de ángulo (`1.256637 + U·0.1`) y
rapidez (`3 + 2·U`) provienen del análisis de `Spider::runAI` en Ghidra; el
runtime compartido usa gravedad 0.16. Durante el estado de salto sigue activa
la regla recuperada de contacto, con 5–10 de daño y cooldown de 30 ticks. La
aproximación de Spider al objetivo y la elección de ramas de IA continúan
siendo provisionales; el ataque de telaraña aún no está implementado.

El VPK 00.97 ya incluye estos cambios: `_local/build-act-one-097/`
`Vagante2Vita-iteration097.vpk` (831,567 bytes, SHA-256
`864A2B5C067EDC0959FCDB030C842F76373646E223F6F203369D13BB22AA3C39`, título
`VAGV00006`, versión `0.97`). La compilación host actual pasó CTest 12/12,
incluido el test que inicia el aviso, verifica el impulso y el daño en el aire,
y comprueba el aterrizaje sobre el suelo. Las unidades afectadas también se
compilaron para ARM Vita con warnings estrictos; la suite Python tenía 170
pruebas aprobadas en la validación de esta build.

La entrega privada `_local/delivery-act-one-campaign097` valida 113 archivos,
tres pisos y 66 imágenes. Los probes físicos con la fuente actual alcanzan
las salidas en 1,741, 736 y 375 ticks; el replay reúne 545 segmentos y 3,184
ticks en los tres pisos. Esto comprueba el recorrido con `player_step`, no
combate, transición entre pisos ni paridad total del generador.

El replay automatizado de campaña fue lanzado con el VPK 00.97 y la instancia
aislada de Vita3K. El proceso terminó con `0xC0000005` antes de crear sesión
nativa; el resultado está en
`_local/vita3k-vagante-test097-campaign/runs/iteration097/run-000001/result.json`.
No hay evidencia de que el juego haya llegado a ejecutarse dentro del emulador.
`campaign_complete` y `native_exit_transition_verified` permanecen falsos.

Queda completar la IA y telaraña de Spider, demostrar combate/loot/trampas y
salidas con el runtime, corregir o sustituir el arranque de Vita3K para poder
reproducir los tres pisos y comprobar la campaña en hardware Vita.
