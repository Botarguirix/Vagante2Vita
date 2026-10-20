# Iteración 01.55: eliminar el retroceso al tocar paredes

> La iteración más reciente es 01.56; esta nota conserva el historial de 01.55.

Se encontró un error real de resolución horizontal: cuando el jugador llegaba
a una pared desde menos de un paso (0.25 px), `axis_move` buscaba el tile de
colisión usando la posición anterior. Si esa posición todavía caía dentro del
tile vacío vecino, el límite calculado quedaba desplazado un tile; eso podía
hacer retroceder al jugador varios píxeles al tocar la pared. Ahora la
corrección busca el tile sólido que intersecta la posición intentada y alinea
el borde del jugador con su cara. Las regresiones cubren el contacto desde
ambos lados, el caso que partía de x=311.75 ante una pared en x=320 y el empuje
sostenido; exigen contacto registrado, sin solapamiento ni retroceso fuera del
borde. Los 13 grupos C host pasan.

El VPK 01.55 se compiló con VitaSDK en
`_local/build-act-one-155/Vagante2Vita-iteration155.vpk` (842,857 bytes,
SHA-256 `6B7EA71A1F9C13F6BA5223CF52C0A243DD8D6D7EF86BD5E740446FEB2FF8904C`). La
suite Python se ejecutó antes de este cambio, con 204/204 pruebas; esta
iteración no altera herramientas ni módulos Python.

Se probó también Vita3K 0.2.1.4190 con un clon privado completo de 2,162
archivos, el VPK 01.54 y la misma campaña/replay: permaneció sin ventana ni
log nativo durante 120 s, por lo que el harness registró timeout antes de la
sesión. La prueba anterior con 0.2.1.4183 terminó con `0xC0000005` en la misma
etapa. Ninguna ejecutó gameplay. Queda pendiente diagnosticar el arranque del
emulador; ACT_ONE aún no está completo ni validado en Vita3K.
