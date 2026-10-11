# Iteración 005: resolución y carga reducidas

## Qué indica el log recibido

runtime.previous.log contiene diez texturas, 2.705 registros, load_us=47636014 (47,636 s) y eventos render_us con cursor/panel. Corresponde al diagnóstico estático 00.02. No contiene BOOT ni PERF del menú animado: no permite cuantificar su consumo CPU. El usuario sí confirmó visualmente las animaciones y observó uso alto; necesitamos el log numerado de esta entrega para medirlo.

## Cambios

- Composición por defecto a **320×180**, ampliación entera 3× a 960×540. L alterna 320×180 y 640×360. Reduce 75% de los píxeles de composición; la UI de texto sigue a resolución de pantalla para conservar legibilidad. No equivale a reducir 75% de todo el consumo CPU.
- Rasterizador recorre sólo los píxeles visibles del destino. Evita recorrer partes de texturas fuera de pantalla y hacer blending de píxeles que se descartarían al reducir resolución.
- Input/espera sincronizados a vblank en vez de consultar el mando unas mil veces por segundo. Lógica mantiene pasos de 60 Hz y presentación tiene objetivo de 30 Hz; rendimiento físico pendiente.
- PERF separa **compose_avg_us**, **scale_avg_us**, **cpu_avg_us**, máximo CPU, FPS de presentación y resolución activa. Cambiar con L reinicia la ventana de medición para no mezclar perfiles.
- Caché opcional **menu-only.vra**: sólo los 22 PNG usados por este menú. 153.245 bytes comprimidos / 224.751 descomprimidos frente a 99.768.748 / 124.879.064 del archivo completo. Conserva exactamente los PNG originales: evita inflar recursos ajenos al menú. No es una compresión de texturas en RAM.

La caché ocupa 5.128.264 bytes RGBA tras decodificar, igual que antes. Ambos framebuffers siguen a resolución de pantalla; no se afirma reducción de su memoria. Comprimir PNG/zlib no reduce por sí mismo el trabajo de blending. Texturas indexadas, RGB565 y GPU son futuras opciones a evaluar con este log.

## Instalar y comparar

1. Instalar `artifacts/iteration005/Vagante2Vita-iteration005.vpk`. ID **VAGV00005**, independiente de 004.
2. Copiar `artifacts/iteration005/menu-only.vra` a **ux0:data/Vagante2Vita/menu-only.vra**. Conservar el data.vra original. Estos recursos privados no están publicados en GitHub.
3. Esperar el título y dejar 20 segundos a 320×180. Pulsar L para cambiar a 640×360 y esperar otros 20 segundos. SELECT muestra estadísticas. Repetir en selección de clase si se desea.
4. Salir con START + SELECT y copiar **ux0:data/Vagante2Vita/logs/iteration005/session-NNNNNN.log**.

El cargador verifica integridad y recursos requeridos de la caché. Si falta o está corrupta, registra el motivo y utiliza data.vra. No recalcula el hash del data.vra completo en consola al usar caché: eso restaría la ventaja de carga. Generar la caché con la misma copia original; su JSON local guarda el hash de origen. Una caché antigua con PNG válidos no garantiza correspondencia con otra versión del juego. Los recursos de habitaciones y gameplay no están en esta caché.

## Verificación

- Archivo completo verificado y caché de 22 recursos aceptada. Cachés truncadas, con bytes sobrantes y checksum corrupto rechazadas.
- Se corrigió el vaciado del inflador cuando quedan datos descomprimidos internos después de agotar la entrada. Una regresión con PNG y relleno sintéticos comprueba el caso sin recursos comerciales, también en CI.
- Render a 320×180 coincide exactamente con muestrear cada dos píxeles la imagen de 640×360 en la prueba del título. Se mantienen animaciones y composición de clases.
- Una ejecución en escritorio: 640×360 **4.123 µs** frente a 320×180 **1.026 µs** por composición, promedio de 20 renders. Es medición de CPU de escritorio, sin escalado, texto, presentación ni GPU: no predice los tiempos de Vita.
- Pruebas de navegación y preservación de logs pasan. VPK Vita Release compilado y paquete verificado. Falta prueba física de 005.

## Reproducir la caché local

`python tools/prepare_menu_archive.py RUTA_DATA_VRA --out _local/menu-only.vra`

El script rechaza sobrescribir su salida; usar otro nombre para generar una caché distinta. Selecciona rutas de la tabla actual de resources.c, verifica recursos y genera metadatos/hash. Al empaquetar una entrega, `tools/package_iteration.py` admite `--menu-archive _local/menu-only.vra`; sólo se guarda en artifacts, excluido de Git.

La reconstrucción de habitaciones, Player, colisiones y arranque de mundo permanece pendiente. Esta entrega optimiza la base gráfica y de carga, sin añadir gameplay.
