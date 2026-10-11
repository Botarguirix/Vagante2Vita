# Iteración 3: menú animado y diagnóstico

El usuario confirmó que la versión 00.02 entró en Vita y mostró el título, aunque estático. Aquella versión componía diez texturas extraídas de data.vra una sola vez; no contenía una captura de pantalla.

## Cambios de 00.03

- Veintidós PNG cargados directamente del archivo original. Integridad y todos los registros comprobados al arrancar. Texturas decodificadas: 5.128.264 bytes, sin contar memoria temporal ni framebuffer; no es medición de memoria máxima.
- Capas de fondo con desplazamiento independiente; atlas de caravana 128×80, ocho fotogramas, siete pasos por fotograma. Luciérnagas 16×16, cuatro fotogramas, ocho pasos por fotograma.
- Dos framebuffers alternados para evitar escribir sobre la imagen presentada. Escalado de 640×360 a 960×540. Objetivo de presentación: 30 Hz; lógica: pasos de 60 Hz derivados del reloj, con límite de recuperación tras pausas largas. El rendimiento real sigue pendiente de medición en consola.
- New Game activa una transición de 90 pasos y abre una **vista parcial de selección de clase**: Knight, Rogue y Mage. Izquierda/derecha recorre circularmente las tres clases base y sus sprites originales. X confirma y registra el bloqueo de arranque del mundo; círculo regresa.

La disposición de tarjetas, la conexión directa a esta vista, la fase de día/noche, trayectorias de luciérnagas y los tintes son aproximaciones del backend nativo. No constituyen recuperación completa del flujo Client/Server ni equivalencia visual. Desbloqueos, variantes, fondos, guardados y clases adicionales siguen pendientes. No se genera una habitación ni se puede jugar.

## Instalar y probar

Instalar build-vita/Vagante2Vita-menu.vpk con VitaShell; VAGV00001, versión 00.03. Mantener data.vra en ux0:data/Vagante2Vita/data.vra. No copiar la vista previa del PC: los recursos se leen en consola.

1. Esperar la carga. Dejar el título animado unos 20 segundos.
2. Cruceta arriba/abajo selecciona. X en New Game abre la vista de clases tras la transición.
3. Izquierda/derecha cambia de clase. X confirma: se muestra el bloqueo real pendiente. Círculo vuelve.
4. SELECT muestra/oculta FPS de presentación y tiempo CPU. START + SELECT sale, y Exit Game también.
5. Copiar ux0:data/Vagante2Vita/runtime.log después de la prueba.

## Cómo leer el log

El archivo se vacía al iniciar y conserva una sesión anterior en runtime.previous.log. Se reemplaza la sesión anterior más antigua; copiar los logs que se quieran guardar antes de volver a arrancar.

| Registro | Qué comprueba |
| --- | --- |
| BOOT | Versión, heap configurado y framebuffer reservado |
| PENDING | Subsistemas todavía pendientes |
| RESOURCE | Ruta real, tamaño comprimido del PNG, dimensiones y bytes RGBA |
| ARCHIVE / archive_ok | Etapa, resultado del inflador y recuento completo |
| LOAD | Resultado y tiempo de carga |
| EVENT | Pantalla, cursor, clase y botones al cambiar estado |
| PERF | Cada dos segundos: presentaciones/s, CPU promedio/máximo y pasos de animación |
| BLOCKER | Confirmación de clase alcanzada; arranque de mundo todavía sin reconstruir |
| WARN / ERROR | Pausas largas, recurso ausente o fallo de presentación |
| EXIT | Salida normal o error de display |

cpu_avg_us y cpu_max_us excluyen espera de vblank y publicación del framebuffer. draw_fps mide presentaciones solicitadas, no FPS de gameplay ni tiempo GPU. SELECT no es un benchmark del juego. Los tiempos absolutos son los del reloj monotónico de Vita.

## Evidencia y verificación

La exportación local muestra MainMenu::step (0x006a4390) con FADE_START y contador 0x5a; al terminar activa readyToStartGame. Client::step (0x0055e500) consume la marca y solicita inicio: esa cadena todavía necesita reconstruirse. PlayerMenu::reset (0x0071ca90) inicializa clases 0, 1 y 2 y consulta desbloqueos. PlayerMenu::left (0x0071b2d0) y getPlayerClass (0x0071e430) usan navegación modular. Los atlas del constructor PlayerMenu (0x0070cba0) tienen celdas de 64×64. El constructor MainMenu (0x006a2ca0) configura caravana y luciérnagas. El pseudocódigo permanece en _local, fuera del repositorio.

Prueba C del controlador: límites, botón mantenido, transición de 90 pasos, navegación circular, confirmación y vuelta. Probe nativo: archivo local de 2.705 registros completo, 22 PNG, diferencia entre fotogramas del título y composición de clases. Cuatro archivos inválidos rechazados. Compilación Vita en Release y VPK verificado como ZIP. No se ha probado 00.03 físicamente.

Compilar como en la iteración anterior. El probe genera preview.ppm, preview.ppm.animated.ppm y preview.ppm.classes.ppm. Estas vistas incluyen composición de recursos; los textos del backend Vita y sus controles se verifican en la consola.
