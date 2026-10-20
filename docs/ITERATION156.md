# Iteración 01.56: aparecer dentro del corredor de la entrada

La captura de Vita3K muestra al personaje junto a la boca de piedra del mundo.
El marcador de entrada del VWI es el tile tipo 10, pero el runtime lo colocaba
en su centro, dejando al jugador pegado al marco. Ahora, cuando el tile tipo 10
tiene un vecino lateral despejado y con apoyo, el spawn elige ese vecino más
cercano al objetivo y orienta al personaje hacia el recorrido. Si ninguno de
los lados cumple la geometría, conserva el spawn anterior.

Se añadieron regresiones para entradas con corredor a la derecha y a la
izquierda, comprobando el punto de aparición, la orientación y movimiento
libre al primer input. Pasan los 13 grupos C host; la suite Python pasa 204/204.
El probe host recalculado completa los tres pisos: primero en 424 ticks,
segundo en 897 y tercero en 5,467, con Goblin King derrotado y 87 HP restantes.
Los recorridos son simulación host y no sustituyen una prueba visual de la
puerta en Vita3K.

VPK compilado con VitaSDK: `_local/build-act-one-156/Vagante2Vita-iteration156.vpk`
(843,110 bytes, SHA-256
`DE323BD859ABD4E864275EC7E498AD462AE40FD506E27ED547B445C254D8425A`). Se
intentó la campaña automatizada con Vita3K 0.2.1.4190 Vulkan/Qt y un clon
privado verificado de 2,162 archivos. El emulador quedó activo 120 s sin crear
sesión nativa; no se ejecutó el VPK ni el gameplay. La prueba previa 01.55 con
OpenGL/Qt terminó antes de la sesión nativa con `0xC0000005`.

La imagen confirma que el runtime mostró el bosque y al jugador junto a la
entrada de piedra en la build que el usuario estaba probando; no identifica por
sí sola el número de versión. El usuario reporta que la puerta lo devuelve. La
prueba host no detecta ese síntoma y Vita3K no pudo ejecutar la 01.56, así que
la puerta y la transición real siguen pendientes de validación en un emulador
funcional.
