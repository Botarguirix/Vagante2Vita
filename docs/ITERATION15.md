# Iteración 015: mundo inicial incluido en la entrega local

## Diagnóstico físico de 014

El log session-000001 muestra aproximadamente 29,7–29,9 FPS estables entre
cambios de pantalla. Las transiciones bajan a 24–28 FPS. Tres intentos de entrada
fallaron porque no existe ux0:data/Vagante2Vita/world/intro-world.vwi.
No se ejecutó física del mundo (logic_steps=0). La transferencia de filas de
014 cuesta ~22 ms; el escalado ~4 ms. No alcanzó el objetivo de 60 FPS.

## Cambios

El VPK local 015 incluye el paquete privado del mundo original: mapa, terreno
y las siete animaciones de cada una de las tres clases. Intenta primero cargar
los archivos externos; si faltan o no cargan, utiliza app0:world. Cada intento
registra WORLD_SOURCE y el resultado. Los datos comerciales permanecen fuera
del repositorio y las compilaciones públicas no los incluyen.

Se conserva la optimización del terreno de 014, pero se retira la comparación
completa de historiales que resultó costosa en la Vita. El escalado escribe filas
preparadas en RAM directamente a CDRAM, como la ruta medida en 013 (~9,34 ms).
Se eliminan ~6,15 MB estáticos de staging/historiales. Se mantiene objetivo
60 Hz y simulación fija de 60 Hz; rendimiento de 015 pendiente de prueba física.
Los campos upload_rows/upload_avg_us de 014 quedan en cero: esa fase separada
ya no existe, y su coste vuelve a scale_avg_us.

Círculo en el mundo vuelve al punto inicial con salud y atributos base, para
repetir recorridos. Registra PLAYER_RESET. Triángulo vuelve a clases.

## Instalación y controles

Instalar Vagante2Vita-iteration015.vpk sobre la misma aplicación VAGV00006.
Conservar menu-only.vra o data.vra que ya utiliza el menú. No hace falta copiar
world aparte para este VPK local. NEW GAME → X → seleccionar clase → X.

Cruceta/stick: andar. X: salto; soltar pronto reduce altura. Abajo: agacharse.
Abajo+X: bajar de plataformas. Arriba/abajo: escalera. L: movimiento lento.
Círculo: reiniciar posición/HP. Triángulo: clases. SELECT: rendimiento.
START+SELECT: salir. Logs en ux0:data/Vagante2Vita/logs/iteration015/.

## Alcance y siguiente paso

Entrega para primer recorrido interactivo de INTRO original, no juego completo.
Andar, salto, agacharse, escaleras, plataformas, colisiones y HP/atributos base
están implementados. Faltan combate, equipo, habilidades, entidades interactivas,
bordes, audio, guardado y transición al primer piso procedural. Primero medir
una sesión física que registre WORLD_START complete=1 y PLAYER; después ampliar
interacciones y reconstruir los movimientos pendientes desde el ejecutable/PDB.

## Validación

Cinco pruebas nativas, seis pruebas Python e integridad comprimida. Probe de las
tres clases con HP 100/80/60, desplazamiento sin solapamiento y render compatible
entre resoluciones. El paquete local debe contener los 23 archivos world y
coincidir en hashes con los recursos originales preparados. Véanse build.log y
validation.log de la carpeta de entrega.
