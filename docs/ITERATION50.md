# Iteración 050: colisión de Boulder con el jugador

La Boulder cuadrada activa ahora colisiona con el jugador mientras cae, cuando
su velocidad supera `2 × gravedad`, como exige `Boulder::doCollision` en la
decompilación Ghidra. La rutina original asigna 20 de daño a una roca cuadrada
normal; el runtime aplica el multiplicador defensivo del jugador, limita el
valor al máximo observado de 30 y registra la roca como ya colisionada con esa
unidad para no repetir el impacto. El daño letal registra `boulder` como causa
de muerte. La roca conserva el movimiento vertical y la detención por tiles de
la iteración anterior.

El área de contacto de la roca usa 15×16 píxeles, según el tamaño de entidad
observado en el constructor. El empuje, el filtrado completo de inmunidades,
daño contra enemigos originales, sacudida/reaparición y paridad exacta del
resolvedor de terreno siguen pendientes. Los enemigos ACT_ONE siguen siendo
provisionales. El primer mundo no se declara completo.

## Verificación

- VPK `00.50` compilado con VitaSDK Windows: 810345 bytes, siete entradas,
  SFO con `TITLE_ID=VAGV00006` y `APP_VER=00.50`.
- SHA-256: `D1F288EFA91DE3C8B38BEA493190915AE9A56F2F1941553324598BE47266FD7D`.
- Las 119 pruebas Python pasan; el bundle privado ACT_ONE 050 valida sus 69
  archivos. El compilador ARM estricto verifica sintaxis y warnings en los
  módulos modificados y en las fuentes de regresión C; esos ejecutables C no se
  corrieron en host porque no hay compilador C host disponible.
- Vita3K no se repitió con el mismo binario/perfil: el intento 049 ya demostró
  el crash nativo de lectura nula antes de ejecutar el juego. No hay evidencia
  de gameplay de 050 en el emulador.
