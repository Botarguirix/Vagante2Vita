# Revisión 010: carga nativa de la intro y flags de colisión

Trabajo independiente de la prueba física 008. Salidas privadas en _local/analysis010; la aplicación instalada sigue siendo la 008. Esta revisión implementa una base portable, no un VPK jugable.

## Evidencia resuelta

ExportWorldTypes.java exporta los miembros de enums del proyecto Ghidra en modo lectura. Level::IntermissionType identifica INTRO=4 e INTRO_RIGHT=9. generateIntermissionLevel crea Room para el acto 9 y ese tipo; getRandomRoom (0x0075f250) selecciona por acto/tipo. Se resuelve la habitación del grupo 4, única variante local.

Los seis campos por celda son: type, backgroundIndex, fargroundIndex, midgroundIndex, foregroundIndex y closegroundIndex. Room::load escribe los cinco índices a offsets 8,10,12,14,16 de Tile; los nombres están en el PDB. Los índices visuales son enteros con signo: se preservan valores negativos como -33, sin reinterpretarlos como índices positivos. Falta reproducir completamente updateAutoTileIndices.

## Implementación

world.c carga un paquete privado VWI1 acotado a 40×24, 960 registros. Verifica cabecera, longitudes, rangos por atlas y ausencia de datos extra. Un fallo no sustituye el mundo ya cargado. world_collision_flags calcula sólidos, plataformas y caras expuestas con límites abiertos/cerrados, basado en generateCollisionFlags (0x00656cd0). Se conservan entrada (14,20) y meta (18,20), y el tipo GOAL que generateIntro añade después de crear las flags.

prepare_intro_world.py convierte únicamente la habitación original local a intro-world.vwi (11.536 bytes), conservando hash de fuente/paquete y entidades en metadatos privados. No empaqueta datos comerciales en GitHub.

starting_stats implementa atributos base de las tres clases, obtenidos de setStartingAttributes (0x0070c7d0), con el orden PlayerStat confirmado por el PDB: STR, DEX, VIT, SPD, STA, INT, LUK. No incluye equipo, afinidades ni antecedentes y no equivale todavía a todas las estadísticas finales.

## Validación y trabajo pendiente

Compilación de escritorio aprobada; tres CTest pasan, incluido el test nuevo de caras de colisión, plataformas, bordes y atributos. El cargador acepta los 960 registros del mapa original. Falta integrar render de tiles, autotiling, entidades/Doodad, cámara, creación física del personaje, controles iniciales e interfaz de estadísticas. No se afirma que el mundo sea jugable aún.

La petición de avanzar autónomamente sigue activa hasta entregar una versión jugable del mundo inicial con movimientos y estadísticas; cada revisión conservará fuentes, logs y datos separados.
