# Roadmap hacia Vagante en Vita

## Iteración 2: título nativo

Verificado en escritorio y compilación Vita: lector C de data.vra por streaming, integridad zlib/Adler, límites de tamaño, rechazo de truncamiento y bytes sobrantes. La copia local contiene 2.705 registros aunque declara 2.702. Se cargan diez PNG originales; no se reserva el archivo descomprimido completo.

El fondo y logotipo se componen a 640×360 y se escalan a 960×540. Se calcula el fondo una vez y se actualiza la pantalla al cambiar la selección. Las cuatro opciones son New Game, Play Online, Options y Exit Game. Las tres primeras muestran avisos de implementación pendiente; la última sale. La cruceta actúa por pulsaciones, sin repetición al mantenerla.

VPK 00.02 compilado. La prueba física anterior validó sólo el diagnóstico 00.01: **esta iteración todavía necesita prueba en Vita**. No se recuperó gameplay. Fuente, tintes, caravana y composición son aproximados; faltan animaciones, parallax y audio. No ejecutamos directamente el menú original descompilado.

## Hitos pendientes

1. **Validar título en Vita:** comprobar carga, colores, selección y salida; recuperar runtime.log. Medir memoria máxima y separar CPU de espera de vblank. render_us incluye espera y presentación, y no representa FPS del juego.
2. **Completar presentación:** recuperar Sprite/Animation, atlas, tintes día/noche, intro y temporizadores; comparar fotogramas con PC. Integrar fuente, signpost y audio originales. Evaluar GPU con mediciones.
3. **Selección de personaje:** seguir callbacks de New Game, reconstruir clases, desbloqueos e interfaces. Criterio: elegir personaje y obtener semilla reproducible.
4. **Una habitación funcional:** recuperar RNG, generación mínima, tiles, Player/Entity, movimiento, colisión y cámara. Comparar estados con semilla y entradas fijas en PC y luego Vita.
5. **Combate y progreso mínimo:** enemigo, daño, objetos, transición y persistencia. Criterio: recorrido mínimo sin corrupción.
6. **Ampliar y optimizar:** generación completa, clases, enemigos, objetos y jefes. Medir CPU/GPU/memoria en consola; cache acotada, atlas y streaming de audio.
7. **Funciones adicionales:** opciones, localización, controles y red; adaptar dependencias PC/Steam.

No hay fecha ni porcentaje fiable. Los 30.568 cuerpos de Ghidra incluyen bibliotecas y no equivalen a funciones reconstruidas.

## Evidencia local

MainMenu: constructor 0x006a2ca0, updateStrings 0x006a3c00, drawBackground 0x006a4ba0, draw 0x006a5b50, up 0x006a6790, down 0x006a67d0, action 0x006a6810. Pseudocódigo exclusivamente en _local/decompiled. La siguiente investigación debe recuperar los destinos de los callbacks de action para abrir selección de personaje.
