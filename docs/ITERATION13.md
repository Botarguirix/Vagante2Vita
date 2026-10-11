# Iteración 013: primera integración del mundo en Vita

Actualiza VAGV00006 / Vagante2Vita a 00.13. Conserva la burbuja, icono, LiveArea, entregas y logs anteriores. Las revisiones 009–012 son análisis/bases nativas separados; 013 vuelve a entregar un VPK.

## Instalación

Instalar Vagante2Vita-iteration013.vpk sobre la aplicación existente. Copiar la carpeta world de esta entrega a **ux0:data/Vagante2Vita/world/**, conservando su estructura. Mantener menu-only.vra o data.vra junto a ella. Los recursos world son privados, preparados desde la copia local del usuario; no están en GitHub. El VPK solo no contiene los gráficos comerciales.

New Game → elegir clase → X abre la habitación INTRO original (acto 9, grupo 4). Si falta un archivo del mundo, aparece el bloqueo y se registra su ruta. No hay generación procedural del primer piso todavía.

## Controles del mundo

- Cruceta izquierda/derecha o stick izquierdo: andar.
- X: salto; soltar antes reduce altura.
- Abajo: agacharse. Abajo+X: atravesar plataformas de una dirección.
- Arriba/abajo junto a una escalera: subir/bajar. X permite abandonarla.
- Arriba en el suelo: mirar arriba. L: andar despacio.
- Triángulo: volver a clases. Select: estadísticas de rendimiento. Start+Select: salir.

En el menú, L conserva cambio de resolución y R cambia calidad; en el mundo, la resolución elegida se conserva. HP y siete atributos base aparecen en pantalla.

## Alcance comprobado

Compilación VitaSDK aprobada. Cuatro pruebas nativas pasan. El probe del runtime carga terreno y animaciones de las tres clases, verifica desplazamiento sin interpenetración y compara render 320×180 con el muestreo de 640×360. También pasan las pruebas de archivo e integridad previas. Falta probar 013 en Vita; el escritorio no valida FPS ni controles físicos de consola.

El terreno es una composición previa de los atlas de la habitación original, obtenida del CSV del cargador/autotiling nativo. Durante la ejecución se dibuja el jugador sobre ese terreno en caché: no es una imagen que simule movimiento. No incluye todavía doodads, iluminación original ni oclusión de capas sobre el jugador. Física reconstruida parcialmente: no se afirma paridad completa con PC.

Esta entrega proporciona el primer recorrido interactivo del mundo, pero **todavía no cumple todos los movimientos y estadísticas solicitados**: faltan rodar, colgarse/trepar bordes, combate/armas, equipo, habilidades, modificadores de antecedentes, interacción con la entrada y estadísticas derivadas completas. El objetivo automático permanece activo.

## Logs

ux0:data/Vagante2Vita/logs/iteration013/session-NNNNNN.log. WORLD/WORLD_RESOURCE/WORLD_START diagnostican mapa, PNG y spawn. PLAYER_EVENT registra cambios de entrada/movimiento y contactos; PLAYER registra posición/velocidad/estado/HP cada ventana. PERF separa dibujo, composición, escalado, pasos de lógica y tiempo total de lógica. Los logs previos no se reemplazan.

Probar las tres clases, caminar en ambas direcciones, salto corto/largo, plataformas, agacharse y escaleras; conservar el log de cada sesión. Continuaremos la recuperación de movimientos restantes sin esperar esa prueba.
