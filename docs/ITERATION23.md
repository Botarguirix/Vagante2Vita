# Iteración 023: arco de Rogue y flechas

## Reconstrucción y alcance

El arco inicial usa seis keyframes de player-draw-bow, propiedades del arma,
origen del proyectil y atlas de la copia local del usuario. El preparador
produce intro-bow.vwb y sprites privados externos; ninguno se publica ni se
integra en el VPK. Weapon::calculateProjectileVelocity (0x007fd620) confirma
velocidad de 0.1 en el primer cuadro y después
2 * ((cuadro + 1) - tiempoRestante / duraciónCuadro) + 2. La carga completa
alcanza 10 píxeles por paso. Sprite::setFrame (0x007b9c20) limita el índice al
último cuadro disponible; se conserva ese comportamiento en la pose de arco.

Rogue puede cargar, apuntar y disparar con su Bow inicial. El disparo consume
una de sus 30 flechas; SELECT muestra la cantidad restante. Las flechas tienen
colisión de 2×2, gravedad 0.16 y rozamiento horizontal 0.99 por paso. Se usa un
barrido de colisión contra terreno para evitar atravesar paredes finas y para
comprobar el trayecto entre la mano y la salida del arco. El pool tiene 32
proyectiles y evita nuevas asignaciones por fotograma. Una flecha incrustada
permanece 120 pasos antes de retirarse. El render solo recorre el rectángulo
pequeño de cada sprite rotado, manteniendo la resolución reducida por defecto.

La adaptación de barrido y el apuntado discreto requieren comparación con PC.
No hay aún daño contra enemigos, recuperación de flechas, reglas completas de
rotura, efectos mágicos de Rod ni equivalencia completa del combate. No se
considera una versión completa del juego. Generación del primer piso, entrada
a la mazmorra, entidades activas, audio, inventario interactivo y guardado siguen
pendientes. La ficha continúa siendo consultable.

## Instalación y controles

Instalar Vagante2Vita-iteration023.vpk sobre la misma aplicación VAGV00006.
Copiar la carpeta **world completa** de esta entrega a
ux0:data/Vagante2Vita/world/. Añade intro-bow.vwb, rogue/aim.png,
rogue/aim_arm.png y entities/dart.png. Mantener menu-only.vra/data.vra externos.
El VPK contiene solamente ejecutable, identidad y arte propio de LiveArea.

En el mundo, **R mantenido** carga el arco de Rogue; soltar dispara. Arriba o
abajo durante la carga apunta a -45° o +45°; izquierda/derecha cambia dirección.
Sin dirección vertical dispara horizontal. R en el menú conserva su función
de cambiar perfil visual. Knight y Mage no reciben un arco inventado.
Cuadrado conserva ataque base; X salto; L movimiento lento; SELECT ficha;
START pausa; Círculo reinicia la intro, repone equipo inicial y elimina flechas;
Triángulo vuelve a clases. Pausa y ficha detienen también los proyectiles.

Los logs van a ux0:data/Vagante2Vita/logs/iteration023/session-NNNNNN.log.
BOW_DATA confirma lectura, BOW_EVENT registra disparos, munición, impactos y
retirada; BOW_BLOCKED informa ausencia de datos, equipo o un estado incompatible.
No se incorporan métricas internas de rendimiento.

## Validación

Nueve pruebas CTest pasan: incluyen lector de arco, rechazo de truncados,
valores no finitos y basura final sin destruir datos previos; carga, liberación
única, velocidad, munición insuficiente, origen reflejado, pool lleno, barrido
contra pared fina y salida obstruida, caducidad y regresiones de movimiento.
El recorrido con datos originales prueba dos disparos de Rogue, horizontal a
derecha y diagonal a izquierda, con munición 30→28 y recuperación terminada.
El render coincide entre 640×360 y la muestra equivalente de 320×180, también
para ataques y movimientos de las tres clases. Seis pruebas Python e integridad
del archivo pasan. VitaSDK genera el VPK. Las imágenes de carga/disparo se
inspeccionan localmente; rendimiento y comportamiento físico 023 pendientes.

## Siguiente dependencia

Conectar las interacciones y la transición de INTRO con generación de pisos de
tamaño variable, y aplicar los impactos a entidades con sus reglas originales.
El objetivo completo continúa activo; el log de esta entrega permite comprobar
las nuevas acciones sin presentar los tiempos del PC como FPS de Vita.
