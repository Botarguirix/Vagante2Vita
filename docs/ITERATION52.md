# Iteración 052: hitboxes ACT_ONE por tipo nativo

Los cuatro tipos provisionales de enemigo ya usan las dimensiones de cuerpo
observadas en sus constructores de Vagante: Goblin 7×12 px (`Goblin::Goblin`,
`0x005db310`), Bat 9×10 px (`Bat::Bat`, `0x004fbf70`), Lurker 10×10 px
(`Lurker::Lurker`, `0x0069cf60`) y Slime 10×7 px (`Slime::Slime`,
`0x0078f0c0`). La caja correcta se aplica a golpes melee, contacto con el
jugador, persecución y apoyo vertical. Antes los cuatro compartían una caja
12×12 px.

Se añadieron pruebas C para comprobar que tocar justo el borde no cuenta como
impacto y que una superposición de medio píxel sí lo hace, para cada subtipo.
Estas pruebas se compilaron para ARM, pero no se ejecutaron en este entorno.
El movimiento y el daño todavía son aproximados; este cambio no reclama IA ni
combate nativos.

## Verificación

- `world_combat.c`, `world_entities_test.c` y `player_test.c` aceptan la
  compilación estricta ARM con `-Wall -Wextra -Werror -UNDEBUG`.
- Suite Python: 119 pruebas.
- Bundle privado ACT_ONE: 69 archivos, manifiesto válido.
- VPK: `00.52`, título `VAGV00006`, siete entradas y sin recursos comerciales;
  810,693 bytes, SHA-256 `BA9FF6F8F998E887CFEB5B592ECD24C08C000053E6B523A272C208DB800555A2`.
- Las pruebas C host no se ejecutaron; solo se verificó compilación ARM estricta.
- Smoke ACT_ONE: se generó replay de 32 pasos para la ruta de 59 tiles y se
  probó con Vita3K aislado 0.2.1.4183 (SDL, instalación por argumento y por
  título) y 0.2.1.4170 (SDL y Qt, con filesystem clonado del perfil Vagante).
  Cada proceso quedó sin ventana, sin `vita3k.log` y sin log nativo; el harness
  agotó 90–120 segundos con salida `-1`, stdout/stderr vacíos. Por tanto, esta
  compilación aún no tiene validación de gameplay en Vita3K. No se accedió a la
  instalación compartida `G:\Vita3k`.

## Límites

Los puntos de aparición, la IA, los ataques, los valores de daño, los drops y
la paridad de generación continúan provisionales. Las dimensiones derivan de
los parámetros visibles de los constructores x86 en la exportación local de
Ghidra; no demuestran por sí solas que todas las animaciones usen la misma
caja durante cada estado.
