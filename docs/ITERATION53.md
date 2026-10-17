# Iteración 053: hitboxes de flecha por enemigo

La detección de flechas ahora comparte la función de dimensiones de cuerpo
ACT_ONE que ya usan melee, contacto y movimiento. Las medidas proceden de los
constructores locales exportados por Ghidra: Goblin 7×12 px (`0x005db310`),
Bat 9×10 px (`0x004fbf70`), Lurker 10×10 px (`0x0069cf60`) y Slime 10×7 px
(`0x0078f0c0`). Antes, el arco trataba a todos como una caja de 12×12 px.

`tests/bow_test.c` comprueba las cuatro medidas y que el borde exacto de la
caja no golpea, mientras que una superposición de medio píxel sí registra el
impacto, tanto en el eje horizontal como en el vertical.

## Verificación

- Compilación ARM estricta: `bow.c`, `world_combat.c`, `bow_test.c`,
  `world_entities_test.c` y `player_test.c` con `-Wall -Wextra -Werror`.
- Suite Python: 119/119.
- VPK `00.53`: 810,743 bytes, siete entradas; SHA-256
  `CDF3A56CEF30E54A5110581F07D3B692DCB4377FABE22937E806F535B46C0C3E`.
- Reconstrucción reproducible desde el árbol actual en
  `_local/build-act-one-053-current/`: siete nombres y siete contenidos
  internos idénticos al VPK 053 original, incluidos `eboot.bin` y `param.sfo`.
  El hash del contenedor reconstruido es
  `7D52AD6E49003E0AE601E62EC245A4B19A2AAA74BD60D4F97C2A1BC11FDDDA65`; cambia
  por marcas de tiempo ZIP.
- El test C se compiló como sintaxis ARM, pero no se ejecutó en host: no hay
  compilador C de escritorio disponible y WSL sigue sin estar accesible.
- Revisión repetida del mismo VPK y entrega privada (67 archivos): se
  prepararon la replay general de 52 pasos y la replay de salida ACT_ONE de 32
  entradas. Vita3K 0.2.1.4170 agotó 90 segundos en ambas configuraciones:
  SDL/Vulkan y Qt/OpenGL. En ambas devolvió `-1`, sin sesión nativa ni contenido
  en `stdout.log`, `stderr.log` o `vita3k.log`; los `result.json` registran
  `process_timed_out=true`. Esto no valida ni refuta el gameplay del VPK: la
  ejecución queda detenida antes de que el juego produzca su primer log.
- Esta revisión compiló con ARM estricto los módulos runtime modificados y
  ejecutó la suite Python 119/119. La prueba C de gameplay no se ejecutó en
  host: no hay compilador C de escritorio y WSL no permite iniciar en este
  entorno.

## Límites

Los tamaños se usan como cuerpos rectangulares uniformes; las cajas dinámicas
de animación no se recuperaron. La IA, el daño y los valores de salud siguen
provisionales. La apertura de cofres aún no integra la generación nativa de
objetos ni el inventario, y la generación de pisos no tiene paridad completa.
