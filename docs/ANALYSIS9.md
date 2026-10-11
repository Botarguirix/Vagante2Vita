# Análisis 009: habitaciones mientras se prueba la 008

Entrega de análisis independiente en _local/analysis009. No genera otro VPK ni modifica la aplicación instalada. Conserva el análisis 008 y sus datos.

Se extendió audit_world_rooms.py para contar tipos de tiles y crear vistas SVG/HTML de todas las habitaciones INTERMISSION: 15 mapas, cada uno con 960 celdas. El informe conserva el hash de rooms.json. Se verificaron la sintaxis XML, el número de celdas y los totales por tipo.

Level::generateCollisionFlags (0x00656cd0) inicializa sólidos para tipos 1/7 y una bandera distinta para tipo 6; después calcula caras expuestas con vecinos y límites abiertos. Los informes etiquetan únicamente la clasificación base. Room::load transforma algunos tipos especiales antes de crear el Level: la geometría cruda no sustituye la carga original, las colisiones completas ni la física.

La previsualización usa colores de diagnóstico, no gráficos del juego. Se incluye cada grupo para evitar asumir cuál corresponde a INTRO. Datos y mapas permanecen privados.

Próximo trabajo: resolver el enum INTRO y selección de grupos en RoomDatabase, nombres/tipos de los seis campos y su transformación al Level, flags de borde y atlas. Después, reconstruir el subconjunto de Player::step para caminar/saltar. La prueba de rendimiento de 008 en Vita puede realizarse en paralelo a este análisis local.

```powershell
python tools/audit_world_rooms.py _local/assets/rooms.json --output _local/analysis009/rooms-report.json --preview-dir _local/analysis009/geometry
```

Las rutas de salida deben ser nuevas para preservar resultados previos. El HTML se abre desde geometry/index.html.
