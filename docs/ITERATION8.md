# Iteración 008: reducir el coste por fotograma

Actualiza la misma aplicación **VAGV00006 / Vagante2Vita**, versión 00.08. Conserva el icono y LiveArea. Entrega independiente en artifacts/iteration008; conserva las anteriores.

## Evidencia física de 006

session-000001.log: carga completa en 644.228 microsegundos, unos 9,2–9,8 FPS; composición aproximadamente 69–72 ms y escalado 23–25 ms por dibujo. No se registran fallos de archivo o display y la salida es normal. Comprimir más PNG no corrige este coste por fotograma. No hay sombras dinámicas implementadas: el sombreado que se ve está en las imágenes originales.

## Cambios

Modo ligero por defecto a 320×180: una caché RAM acotada de 921.600 bytes conserva las capas originales del paisaje, el título y las tarjetas. Reconstruye al cambiar pantalla, clase o resolución. La caravana y los tres personajes siguen animados; el paisaje queda fijo, sin parallax, ciclo de tintes ni luciérnagas. La caravana se dibuja encima del paisaje en este modo simplificado; la composición original sigue disponible para comparación. No se cambian ni recomprimen los recursos originales.

El escalado prepara cada fila en RAM y la copia al framebuffer; evita leer del framebuffer para duplicar filas. Los plazos de dibujo avanzan en una cadencia de 33.333 microsegundos, en vez de reiniciar el temporizador en cada dibujo. Esto reduce una causa de cadencia irregular; no garantiza 30 FPS físicos.

## Prueba en Vita

Instalar Vagante2Vita-iteration008.vpk sobre la aplicación existente. Mantener menu-only.vra o data.vra en ux0:data/Vagante2Vita. No hace falta copiar nuevos recursos.

- R: alternar modo ligero/composición completa.
- L: alternar 320×180/640×360.
- Select: mostrar FPS/tiempo CPU. Start+Select: salir.
- Probar título y selección de clases durante al menos 10 segundos por perfil; esperar 2–4 segundos tras alternar para obtener ventanas estables.

Log nuevo: ux0:data/Vagante2Vita/logs/iteration008/session-NNNNNN.log. PERF añade lightweight, cache_rebuilds y frame_interval_max_us, además de composición/escalado. La primera reconstrucción de caché y las transiciones pueden elevar CPU máximo; comparar ventanas estables. Los logs anteriores permanecen intactos.

## Validación

VPK compilado con VitaSDK/GCC 15.2. Pruebas de archivo/integridad, transiciones y conservación de logs aprobadas. El probe verifica animación en caché, ausencia de rastros al reconstruir, invalidación por clase, equivalencia entre resoluciones y fade a negro. En escritorio: composición completa reducida 1.569 µs frente a 14 µs por dibujo ligero en esta ejecución. **Estos tiempos no son FPS de Vita**; falta medir la 008 en consola.

El mundo y el jugador todavía no están implementados. [Ruta de primera prueba jugable](FIRST_GAMEPLAY.md).
