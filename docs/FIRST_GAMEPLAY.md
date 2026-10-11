# Primera prueba jugable: zona inicial original

Los archivos actuales bastan para iniciar su reconstrucción. El menú anima sprites y permite elegir clases; todavía no contiene un jugador con movimiento físico, saltos o colisiones. 008 optimiza la presentación y mantiene el bloqueo explícito al confirmar clase.

## Ruta recuperada

GameEngine::newGame (0x005c1570), LevelGenerator::generateIntro (0x00687c40) y generateIntermissionLevel (0x006877f0) muestran la ruta original. La última consulta RoomDatabase, usa las dimensiones del acto 9, selecciona una habitación e inicializa tiles; después llama a generateCollisionFlags, generateDisplayIndices y createRoomEntities.

El rooms.json local contiene INTERMISSION, acto 9, de **40×24 tiles**, con 15 grupos de una habitación cada uno. Cada fila contiene 40 registros de seis enteros; Room::load (0x007579d0) consume seis campos por tile. Hay que resolver qué grupo corresponde a INTRO y el significado de esos campos; el grupo 0 no está confirmado. La herramienta audit_world_rooms.py valida esta estructura y produce un informe privado sin copiar mapas al repositorio.

generateIntro marca los tiles explorados, coloca una meta de tipo 11 en (w/2-2,h-4) y establece entrada/salida en (w/2-6,h-4): (18,20) y (14,20) con estas dimensiones. Son coordenadas de tiles, no la posición física del jugador. Partículas y apariencia estacional pueden omitirse en la primera prueba.

## Siguientes pasos

1. Recuperar RoomDatabase::loadRooms (0x0075dd40), Room::load y Level::initRoom (0x00655be0): grupo INTRO, campos/flags, transformaciones y atlas. Criterio: cargar y comparar los 960 tiles con PC; no deducir colisiones del color.
2. Renderizar tiles y cámara con caché/culling. Criterio: mostrar la habitación original, entrada y meta en sus ubicaciones, registrando recursos ausentes.
3. Recuperar generateCollisionFlags (0x00656cd0/0x00656fa0), índices visuales (0x00657850) y el movimiento/salto de Player::step (0x006e3740): cuerpo físico, gravedad y suelo. Criterio: caminar y saltar sin atravesar paredes, registrar posición/velocidad/contactos y comparar entradas reproducibles.
4. Unir clase seleccionada, creación/colocación de Player y habitación. **Primera prueba jugable:** elegir clase, aparecer en la zona original, caminar, saltar y salir sin fallo. Medir lógica y dibujo por separado, con objetivo de 30 FPS.
5. Añadir interacción de salida y primer piso procedural con RNG/semilla. Después, combate, objetos, guardados y red. La primera habitación puede probarse antes de reconstruir toda la generación procedural.

## Qué necesitamos del usuario

Para analizar no faltan archivos: EXE, PDB y VRA ya están disponibles localmente. Para comprobar rendimiento necesitamos el log de 008 después de probar R (ligero/completo) y L (resolución). Más adelante compararemos la zona inicial con PC cuando el render de tiles esté preparado.

```powershell
python tools/audit_world_rooms.py _local/assets/rooms.json --output _local/world-room-audit008.json
```

Pseudocódigo y datos comerciales permanecen privados en _local. El informe de estructura todavía no implementa un mundo en Vita.
