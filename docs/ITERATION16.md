# Iteración 016

La sesión física 015 confirma entrada al mundo con Knight y Rogue desde
ux0:data/Vagante2Vita/world. Durante el recorrido registra ~58–59 FPS,
HP 100/80 y movimiento, salto, agacharse, escaleras y reinicio. Salida normal.
Algunos aterrizajes alternaban FALL/STAND por un pequeño hueco de colisión.

## Cambios

- Aterrizajes ajustan los pies al borde superior de la superficie, validando
  que el ajuste no penetre sólidos. Prueba de 180 pasos sobre plataforma
  exige apoyo continuo y posición exacta. No se afirma paridad completa con PC.
- START pausa/reanuda la física del mundo. Triángulo sale de pausa a clases;
  Círculo reinicia únicamente mientras se juega.
- Sin contador ni muestreo interno FPS/CPU/PERF. SELECT ya no activa métricas.
  HP y atributos son información del personaje y se conservan. Logs de carga,
  errores, eventos, pausa y estado de jugador siguen activos.
- VPK exclusivamente ejecutable y recursos propios de la aplicación.
  Se elimina empaquetado/carga app0:world; herramientas de entrega rechazan
  recursos adicionales no autorizados. Mundo y caché de menú van aparte.

## Instalación

Actualizar con Vagante2Vita-iteration016.vpk, misma burbuja VAGV00006.
Conservar world completo en ux0:data/Vagante2Vita/world/ y menu-only.vra
(o data.vra) en ux0:data/Vagante2Vita/. El log 015 confirma que estos recursos
externos ya están instalados: no hace falta volver a copiarlos.
Si faltan, la entrega contiene world separado para copiarlos.

NEW GAME → X → clase → X. Cruceta/stick anda, X salta, abajo agacha,
abajo+X atraviesa plataformas, arriba/abajo usa escaleras, L camina lento.
START pausa, Círculo reinicia, Triángulo vuelve a clases, START+SELECT sale.
Logs nuevos: ux0:data/Vagante2Vita/logs/iteration016/.

## Validación y pendiente

Cinco pruebas nativas y seis de archivo Python, integridad comprimida y
recorrido de las tres clases aprobados. Compilación VitaSDK correcta.
Comprobar en Vita pausa/reanudación y aterrizajes, sobre todo plataformas.
Las métricas futuras deben proceder del plugin externo de consola; PowerShell
puede procesarlas cuando estén disponibles, no mide por sí solo FPS de Vita.
Todavía faltan bordes, combate, entidades/interacciones, equipo, audio,
guardado y transición al primer piso procedural.
