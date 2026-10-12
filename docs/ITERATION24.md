# Iteración 024: mapas de tamaño variable y cámara

## Cambio de runtime

Se conserva la intro original y sus controles. El cargador ya no impone 40×24:
lee VWI2 con dimensiones y entrada/salida explícitas, valida límites y conserva
el World anterior si falla. VWI1 sigue aceptado con su comportamiento de INTRO.
La etapa temporal se reserva en heap para no poner un World grande en la pila
Vita. Capacidad actual: 4096 tiles, hasta 128 por eje, suficiente para ACT_ONE
80×40 y TUTORIAL 80×48. No cubre todavía todos los actos/mapas grandes; se
necesitará ampliar almacenamiento o segmentarlo conforme avance la generación.

La cámara sigue al jugador en ambos ejes, limita el viewport al tamaño real y
usa coordenadas pares compartidas por ambas resoluciones. Terreno, personajes,
decorado, arma y flechas usan la misma traslación. No se añade un mapa de prueba
al juego. El menú continúa con perfil ligero por defecto.

El terreno decodificado se convierte en imagen opaca en su propia memoria.
Se elimina la segunda copia completa de 640×384 (983 040 bytes en la intro);
solo se reserva además la versión reducida. El blit copia las filas visibles,
con relleno únicamente donde falta terreno. El lector PNG acepta hasta 2048
por eje y 1 048 576 píxeles totales. El tamaño de la textura debe coincidir con
el World: rechaza paquetes/texturas incompatibles. La caché considera carpeta,
clase y dimensiones, evitando reutilizar terreno de un mapa anterior.

## Contrato VWI2

Cabecera de 32 bytes, little-endian. Primera parte: magic VWI2 (4 bytes), ancho
uint16, alto uint16, flags uint32 (bit 0 cielo abierto, 1 izquierda, 2 derecha),
count uint32 igual a ancho×alto. Segunda: entrada X/Y y salida X/Y uint16,
seguido de dos uint32 reservados a cero. Después count registros de 12 bytes:
tipo uint8, reservado cero uint8 y cinco índices int16. Mismos límites de capas
que VWI1. Entrada/salida dentro de mapa y libres de sólido. Sin basura final.
El archivo no implementa entidades, habitaciones, semilla ni reglas de piso;
es un contrato de carga para el ensamblado futuro. El VWI1 de INTRO conserva
su modificación de salida después del autotiling; VWI2 conserva tipos dados.

## Validación

Diez CTests pasan. El test nuevo carga 80×40, comprueba última celda y límites,
entrada/salida, cámara horizontal/vertical, centros no finitos y cuantización;
rechaza flags desconocidos, count distinto, entrada fuera, spawn sólido,
truncado, basura final y capacidad excesiva sin modificar World anterior.

El probe de render usa un terreno sintético de 1280×640 solamente en _local:
cuatro posiciones de cámara verifican el pixel correspondiente al mundo y
la igualdad del render reducido con la muestra del render 640×360. También
verifica movimiento/colisión más allá del ancho viejo de 640, cambio de carpeta
con la misma clase y rechazo de textura de tamaño incorrecto. Las regresiones
con recursos originales prueban movimiento/estocada de tres clases y dos
disparos de Rogue con munición 30→28. Doce pruebas Python e integridad del
archivo pasan. Render original y fixture inspeccionados; VitaSDK compila.
Rendimiento y comportamiento físico de 024 aún pendientes, sin métricas internas.

Fixture reproducible (Pillow y paquete privado del usuario):

```powershell
python tests/prepare_extent_fixture.py _local/world-runtime023-bundle _local/extent024-fixture
```

En build HOST_PROBE, ejecutar:

```text
extent-render-probe _local/extent024-fixture/intro-world.vwi _local/extent024-fixture _local/extent024.ppm _local/world-runtime023-bundle
```

Los paths y logs de HOST_PROBE se distinguen de la consola; el fixture no se
incluye en world de la entrega ni es una mazmorra implementada.

## Instalación y siguiente dependencia

Vagante2Vita-iteration024.vpk actualiza VAGV00006, versión 00.24. Sin recursos
originales dentro del VPK. La carpeta world entregada es la original usada en
023: si ya está completa instalada, sirve para 024; en otro caso copiarla a
ux0:data/Vagante2Vita/world/. menu-only.vra/data.vra permanecen externos.
Controles conservados: R carga/suelta arco Rogue, Cuadrado estocada, X salto,
L lento, SELECT ficha, START pausa, Círculo reinicio, Triángulo clases.
Logs separados ux0:data/Vagante2Vita/logs/iteration024/session-NNNNNN.log.
WORLD registra dimensiones, entrada/salida y formato leído, además de eventos
existentes. No se añade FPS/CPU al juego.

Falta el ensamblado de habitaciones, consumo original del RNG, autotiling del
piso completo, contenido por acto, transición de INTRO y preservación del
personaje entre pisos. Combate con entidades, magia, audio, menús completos y
guardado siguen pendientes. Esta revisión quita el límite técnico de tamaño
para el primer piso; no afirma que ya esté generado ni que el juego sea completo.
