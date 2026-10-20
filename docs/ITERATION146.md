# Iteración 01.46: estado persistente del roster de jefes

El estado runtime de ACT_ONE ahora conserva entre pisos una máscara de derrotas
para Goblin King (49), Dragon (50) y Worm (51), identificadores del selector
`LevelGenerator::addBoss` en Ghidra. Las bajas desde melee y flechas usan el
mismo registrador. La salida del piso final requiere primero que el jefe local
esté derrotado y después que la máscara sea `0b111`; los logs distinguen el
roster incompleto y el checker del smoke rechaza una finalización con bits
faltantes. La prueba de flujo cubre ambos gates y las pruebas de entidades,
arco y checker cubren registro y validación.

La distribución exacta de los tres encuentros por piso todavía no está
certificada: la selección nativa depende de estado RNG previo que la generación
reconstruida aún no reproduce. Tampoco hay IA Dragon ni combate Worm (sus 20
segmentos están pendientes), por lo que la campaña actual no puede alcanzar
legítimamente la máscara completa. El gate evita informar como terminado un
mundo parcial; no convierte aún el contenido provisional en la campaña jugable.

Validación: 13 pruebas C host; 203 pruebas Python y 70 subtests. El VPK
VitaSDK Windows 01.46 compila en `_local/build-act-one-146/` con identidad
`VAGV00006`. La ejecución Vita3K sigue pendiente; la instancia ha agotado
plazos antes de generar log nativo en iteraciones previas. No se reinstaló ni
copió la instancia completa en esta iteración.

Los tres route proofs y sus trazas se regeneraron con huella
`b208f09e896a94a2b80a338dc66187f90e3eeb1e8a273695304282ce584a6805` y el
bundle local actualizado está en `_local/analysis146/campaign-delivery/`.
Las rutas siguen siendo pruebas host; solo el piso 2 incluye la derrota
provisional del Goblin King y no prueban la campaña runtime completa.
