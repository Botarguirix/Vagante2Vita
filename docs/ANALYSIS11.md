# Revisión 011: autotiling y terreno inicial

world_auto_tiles reconstruye la selección de variantes de Room::updateAutoTileIndices (0x007559a0), incluida la comparación de vecinos, recorrido por columnas, familias de farground y regla de midground sólido. El probe nativo exporta CSV de los 960 tiles ya procesados. La rutina es una reconstrucción revisada, todavía sin comparación de ejecución instrumentada con PC.

render_world_probe.py dibuja las cinco capas usando los atlas originales extraídos. Resultado privado: _local/analysis011/intro-terrain.png y su informe; CSV y native-checks.log en la misma carpeta. Los índices usados caben en sus atlas. La vista muestra terreno de la intro, sin iluminación original, doodads, jugador ni física. No es una captura de un juego ejecutándose.

Pruebas de vecinos de autotile, flags de colisión, plataformas, bordes y atributos aprobadas. El mapa original carga y exporta correctamente después de aplicar variantes. La aplicación física 008 no cambia en esta revisión de escritorio.

Siguiente revisión: cuerpo de jugador 8×12, gravedad de Level 0,16 y salto base 3,5 (constantes observadas), desplazamiento, plataformas, agacharse/escaleras y registro de estados. Todavía faltan equipo, habilidades, interacción y estadísticas finales para alcanzar el objetivo jugable solicitado.
