# Reconstrucción y port

1. **Identificar versión reproducible.** Hash EXE/PDB/VRA, verificar GUID/age, importaciones y símbolos. Guardar original fuera de Git. Completado para la copia Windows local.
2. **Generar inventario y pseudocódigo.** EXE/PDB cargados, análisis finalizado y primera exportación completada. Examinar Player, Entity, Level, ResourceManager y GameSave. Exportación no equivale a reconstrucción; revisar advertencias y fallos registrados.
3. **Recuperar una sección pequeña en PC.** Elegir temporizador/RNG/carga de datos, reconstruir tipos y control de flujo. Comparar con comportamientos observados del juego; documentar procedencia y confianza. No empezar copiando todas las bibliotecas descompiladas.
4. **Capa de plataforma y recursos.** Probar ventana, mando, streaming de audio, textura y shader equivalentes. Mantener la lógica recuperada separada del backend Vita.
5. **Segmento jugable mínimo.** Una habitación, jugador, colisión, una entidad y guardado; semillas e inputs fijos para comparar estado. Primero PC, luego Vita real.
6. **Medir y ampliar.** Registrar tiempos CPU/GPU, memoria máxima y cargas; reducir atlas/cache y evitar reservar todo el archivo. Ampliar niveles, objetos, enemigos y jefes sólo al estabilizar cada subsistema.
7. **Funciones adicionales.** Menús, localización, guardados completos y multijugador. Distribuir ejecutable del port cuando exista con instrucciones para aportar recursos propios.

Estado actual: herramientas e investigación inicial. No se ha recuperado un bucle jugable ni se ha validado equivalencia. No hay estimación fiable de fecha ni porcentaje de decompilación del juego; la cantidad de funciones que Ghidra exporta no es un porcentaje de avance.
