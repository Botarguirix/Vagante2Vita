# Revisión 012: movimiento sobre el mapa inicial

player.c implementa un subconjunto inicial con cuerpo 8×12, gravedad 0,16 y salto 3,5, constantes observadas en Player/Level. Movimiento horizontal con aceleración y límite por SPD, fricción, salto por pulsación (sin repetición al mantener), salto corto al soltar, agacharse con comprobación de techo, plataformas de una dirección, caída mediante abajo+salto y escaleras. Las decisiones de resolución de colisión subdividida y algunas transiciones son adaptación nativa; no se afirma equivalencia completa con Player::step original.

Atributos base y salud máxima de las tres clases están conectados al cuerpo. Salud base: 50+10×VIT, sin equipo, afinidades ni antecedentes. Faltan estadísticas derivadas completas, combate, rodar/habilidades específicas, colgarse/trepar bordes, cámara y presentación interactiva en Vita.

Las pruebas de escritorio comprueban avance, salto/aterrizaje, pulsación mantenida, ausencia de interpenetración, agacharse, paredes, plataformas, caída y escaleras. También recorren el mapa original con entradas fijas. Logs en _local/analysis012. Revisión de base portable; aplicación física aún 008. Siguiente: integración de recursos y personaje en el runtime Vita, manteniendo explícitos los movimientos pendientes.
