# Iteración 019: selección de perfil tras pruebas locales

Log físico 018: perfiles/resoluciones alternados, mundo cargado desde ux0,
salida normal. Sin métricas de plugin adjuntas: no se infieren FPS/CPU.
Sigue faltando hang.png de Knight, con fallback funcional.

## Opciones evaluadas

Tres perfiles × dos resoluciones × tres repeticiones, con 24 cuadros cambiantes
por caso y recursos locales reales. Los tiempos son composición en PC, no Vita.
En la última comparación a 320×180: completo 1152–1244 us, preparado 923–968 us,
ligero 16–17 us. Medianas: completo 1155 us y preparado 941 us, reducción ~19 %.
Una ronda previa obtuvo ~24 %; se conserva la diferencia de condiciones de PC
y no se extrapola a Vita.
El primer prototipo preparado tuvo resultados inconsistentes; se mejoró
el desplazamiento de capas por filas eliminando divisiones por píxel.

El perfil preparado almacena colores/tintes/alfa calculados en RAM para capas
y sprites, conserva desplazamiento y usa cuatro luciérnagas frente a doce.
Fija el tinte nocturno para evitar reconstruir la caché en cada cuadro.
Puede reservar hasta aproximadamente 5 MB adicionales de RAM si se prueban
menús de clases; libera esa caché al entrar al mundo y al salir.
No se alteran archivos originales ni se integran dentro del VPK.

**Selección predeterminada: ligero 320×180**, por rendimiento y antecedentes
físicos de ~58–59 FPS del recorrido. No significa que 019 esté medida en Vita.
R recorre ligero → completo → preparado → ligero en el menú. L cambia resolución.
El mundo usa su terreno preparado anterior; las opciones afectan al menú.
Sin FPS/CPU integrados: medir externamente para decidir si preparado resulta
aceptable en la consola. QUALITY registra perfil 0 completo, 1 ligero, 2 preparado.

## Jugabilidad e instalación

VPK conserva VAGV00006. Mundo y menu-only.vra/data.vra externos como antes.
Para el gráfico de agarre, copiar world/knight/hang.png, rogue/hang.png y
mage/hang.png a ux0:data/Vagante2Vita/world/<clase>/hang.png.
NEW GAME → clase → X. Movimiento, salto, plataformas, escalera, agacharse,
agarre/subida/suelta de bordes, HP y atributos base. START pausa, Círculo
reinicia, Triángulo vuelve a clases; START+SELECT sale.
Logs: ux0:data/Vagante2Vita/logs/iteration019/.

## Validación y alcance

Pruebas nativas, archivos, recorrido tres clases, animación y equivalencia
entre resoluciones. Raster rápido comparado píxel a píxel con compositor de
referencia en varios instantes. Build VitaSDK y validación en entrega separada.
La intro es jugable; este VPK no es aún el juego completo ni su versión final.
Generación de pisos, combate, entidades/interacciones, equipo, audio y guardado
siguen pendientes según GENERATION_RECOVERY.md y ROADMAP.md.
