# Iteración 017: bordes

PDB y Player::step (0x006e3740) confirman hanging, hangTileX/Y y sprites
hang/crawl_ledge. Se implementa un adaptador geométrico nuevo; umbrales,
trayectoria y tiempos no tienen aún paridad verificada con el PC.

Al caer y mantener dirección hacia un borde sólido con espacio arriba,
el personaje se cuelga. Arriba sube si el cuerpo tiene espacio en el recorrido;
abajo suelta y X salta hacia fuera. Una espera de 12 pasos evita reagarre
inmediato. No se habilitan habilidades como rodar para todas las clases.

VPK sin recursos del juego. Copiar los tres archivos nuevos knight/hang.png,
rogue/hang.png, mage/hang.png de world a ux0:data/Vagante2Vita/world/,
o copiar el world completo. Si faltan estos gráficos opcionales se usa climb;
la lógica sigue funcionando y WORLD_OPTIONAL registra el fallback.
Mantener menu-only.vra o data.vra externos. Misma burbuja VAGV00006.

Controles previos conservados: START pausa, Círculo reinicia, Triángulo clases.
Sin métricas integradas. PLAYER incluye hanging y eventos HANG. Logs nuevos:
ux0:data/Vagante2Vita/logs/iteration017/.

Pruebas nativas incluyen agarre por ambos lados, mantener agarre, techo que
impide subir, subida libre, soltar y saltar. Recorrido de tres clases y archivo
comprimido también se validan. Falta comprobación física de estos movimientos.

La petición se amplía al mundo completo. Próximos sistemas: resolver generación
original por actos/semilla, cargar habitaciones de tamaños distintos, conectar
transiciones, entidades, combate, objetos, equipo y progreso. La intro actual
no sustituye esos sistemas y el objetivo todavía no está completado.
