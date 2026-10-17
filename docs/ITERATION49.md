# Iteración 049: caída básica del Boulder

Al liberarse la BoulderTrap en el sexto frame, la roca cuadrada inicia con
velocidad vertical `2 × gravedad`. El paso añade gravedad `0.16` por frame,
limita la velocidad a 12 px/frame y detiene la caja 16×16 sobre el primer tile
sólido debajo. Los valores de liberación y gravedad vienen de las funciones
`Boulder::release` y `Boulder::step` examinadas en Ghidra. La resolución contra
tiles es una aproximación del port: el pseudocódigo disponible de
`Entity::moveIncludingChildren` no incluye su resolvedor de terreno.

La roca se dibuja en su nueva posición. Aún no se implementa colisión ni daño
contra unidades; tampoco se recrea el ciclo de sacudida/reaparición. El primer
mundo sigue incompleto.

## Verificación

- VPK `00.49` compilado con VitaSDK Windows: 809894 bytes, siete entradas,
  SFO con `TITLE_ID=VAGV00006` y `APP_VER=00.49`.
- SHA-256: `8A09551BA343350C65E6A0D487BCC442B10D1C11ECB2383006C70541A4815AA7`.
- Las 119 pruebas Python pasan y el paquete ACT_ONE privado 050 valida sus 69
  archivos. La comprobación estricta del compilador ARM pasa para el runtime y
  la regresión C de entidades.
- Vita3K 0.2.1.4183, aislado en `_local/vita3k-iteration049`, se bloquea antes
  de crear log nativo o sesión de juego. Windows muestra una excepción de
  lectura en `0x00007FFB3D4B5F31` desde `0x0000000000000000`; el harness agotó
  60 segundos y guardó el diagnóstico en
  `_local/vita3k-iteration049/runs/iteration049/run-000001/result.json`.
  Por tanto, el VPK está empaquetado y verificado, pero no validado en emulador.
