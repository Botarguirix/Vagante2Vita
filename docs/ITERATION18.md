# Iteración 018: fondo animado para comparación externa

La sesión 017 registra entrada y recorridos Knight/Rogue, agarres HANG y
salida normal. Faltan knight/hang.png y rogue/hang.png: funcionó el fallback
climb, por lo que no se vio el gráfico específico de agarre.

018 inicia los menús con el perfil animado: capas en movimiento, caravana,
luciérnagas y variación de tinte de la reconstrucción existente. No se afirma
paridad exacta con el tween/ciclo de día original. R alterna con el perfil
ligero durante el menú para comparar ambos en la misma sesión; L alterna
320×180/640×360. El mundo mantiene su terreno optimizado y lógica previa.

No se añaden métricas internas. Registrar rendimiento con el plugin externo
bajo iguales ajustes de frecuencia de consola, resolución y pantalla. Los
logs QUALITY/PROFILE identifican los cambios, pero no miden FPS/CPU.
El perfil completo recompone varias capas por cuadro y puede bajar FPS;
esta entrega permite medir ese coste en hardware, no promete 60 FPS del menú.

Instalar VPK 018 sobre VAGV00006. Todos los recursos del juego van aparte.
Conservar world y menu-only.vra/data.vra externos. Copiar los tres hang.png
incluidos en world a cada carpeta de clase si no se copiaron en 017.
Logs: ux0:data/Vagante2Vita/logs/iteration018/.

Pruebas del render animado comprueban cambio entre cuadros y equivalencia
entre resoluciones; pruebas de movimiento, colisión y archivos se conservan.
Compilación y resultados están en build.log/validation.log. Validación física
018 pendiente. Recuperación de generación continuada y documentada en
GENERATION_RECOVERY.md; no hay todavía primer piso procedural implementado.
