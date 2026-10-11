# Iteración 014: salida de imagen y terreno optimizados

La sesión física 013 aportada mantiene aproximadamente 29,97 FPS en título:
composición ~0,45 ms, escalado ~9,34 ms y dibujo total ~10,05 ms.
El objetivo estaba limitado a 30 Hz. Se observaron reconstrucciones de caché
con máximos ~64 ms y una pausa aislada de ~117 ms entre cuadros.
La sesión no entró al mundo: no permite diagnosticar su rendimiento físico.

## Cambios

- Objetivo de dibujo 60 Hz, conservando simulación a 60 Hz.
- Escalado y HUD en RAM normal. Cada framebuffer conserva su propia copia
  anterior en RAM; solo las filas que cambiaron se transfieren a CDRAM.
  No se lee memoria de pantalla para comparar. Las filas restauran también
  sprites desplazados y textos retirados al alternar los dos buffers.
- Terreno opaco preparado una vez en dos resoluciones, copiado por bloques;
  dibujo del jugador recortado a su rectángulo visible.
- PERF agrega upload_rows y upload_avg_us; cpu_avg_us incluye la transferencia.
- Se destinan aproximadamente 7,82 MB adicionales de memoria estática a
  staging, historiales de pantalla y terreno preparado. Cambios de pantalla
  aún pueden provocar picos al recomponer el fondo; no se promete 60 FPS
  reales hasta medir esta versión en Vita.

## Instalación y prueba

Instalar Vagante2Vita-iteration014.vpk: actualiza la misma burbuja VAGV00006.
Los recursos world y menu-only.vra son los mismos de 013; si ya están instalados
no hay que copiarlos otra vez. Para instalación nueva, copiar world completo a
ux0:data/Vagante2Vita/world/ y menu-only.vra a ux0:data/Vagante2Vita/.

NEW GAME → X → elegir clase → X entra al mundo. Cruceta/analógico mueve,
X salta, abajo agacha, arriba/abajo usa escalera, L camina despacio,
Triángulo regresa a selección. SELECT muestra rendimiento; START+SELECT sale.
Probar también cambios de clase, SELECT y entradas/salidas para detectar
rastros visuales. Logs nuevos: ux0:data/Vagante2Vita/logs/iteration014/.

## Validación

Cinco pruebas CTest, seis pruebas de archivo Python y comprobación nativa
comprimida correctas. Recorrido de las tres clases con HP 100/80/60,
colisiones y equivalencia entre resoluciones correctos. Imagen del recorrido
idéntica byte a byte a 013. Compilación VitaSDK y registro en build.log.

## Pendiente

Es una reconstrucción parcial del mundo inicial: faltan entidades y decorado
interactivo, combate, inventario, afinidades y movimientos adicionales que
requieren reconstrucción, audio, guardado y generación del resto del juego.
El próximo log debe incluir tiempo en el mundo para separar dibujo y física.
