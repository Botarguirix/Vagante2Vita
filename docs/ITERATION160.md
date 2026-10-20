# Iteración 01.60: la cueva vuelve a ser el punto de entrada

El cargador de partida intentaba ACT_ONE antes que el mapa introductorio. Con
ambos paquetes instalados, eso saltaba la escena con la cueva. Ahora las
partidas normales prueban primero `intro-world.vwi`; si no está disponible,
ACT_ONE sigue funcionando como respaldo. Las replays automatizadas conservan
el arranque directo en ACT_ONE para poder recorrer sus pisos sin alterar la
secuencia validada.

El probe de física ejecutado sobre el paquete introductorio local informa:
`entrance=14,20`, `goal=18,20`, recorrido de 5 tiles completado en 38 ticks,
sin salto y sin muerte. Esto confirma que el modelo de jugador puede alcanzar
la meta que representa la cueva; no verifica la transición gráfica en Vita3K.
La transición `floor_index=-1` a ACT_ONE y la retirada del reinicio con
Círculo/B vienen de 01.59.

Verificación de esta iteración: los 13 grupos C host pasan, las 204 pruebas
Python pasan y el VPK se compila como `01.60`, con título `Vagante2Vita`, ID
`VAGV00006`, cabecera SELF y los siete archivos de paquete esperados. El smoke
01.59 en un clon privado completo de Vita3K 0.2.1.4190 expiró a 150 s antes de
crear una sesión nativa; no se inició otra ejecución idéntica porque esa falla
ocurre antes de que Vita3K cargue el ejecutable.

VPK: `_local/build-act-one-160/Vagante2Vita-iteration160.vpk` (843,269 bytes,
SHA-256 `8657850702274BFEA5DB04738944382953068E05F7C0A7DA18A6D6738E59D52A`).
La prioridad nueva de inicio todavía requiere validación de gameplay en
Vita3K o hardware. La reconstrucción ACT_ONE también conserva sus límites de
paridad de generación y contenido provisional descritos en la hoja de ruta.
