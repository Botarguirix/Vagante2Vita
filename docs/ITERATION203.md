# Iteración 203 — feedback visual al recibir daño

El VPK 02.03 conserva el render lógico 640×360 escalado a la zona visible
960×540 de Vita. `Player::hurt` en `0x006f9020` pone `hurtTimer=10` únicamente
después de que `Unit::hurt` acepta el impacto. El runtime aplica daño desde
varios sistemas todavía separados, así que se añade solo una señal visual
temporal: destello rojo durante 10 ticks en impactos de telaraña, trampa,
contacto enemigo y roca. No cambia HP, física ni reglas de invulnerabilidad.

Las 14 suites C host y 226 pruebas Python pasan. VitaSDK Windows compiló el
VPK `VAGV00006` versión 02.03; `check_vpk.py` informa siete entradas, cero
errores y cero advertencias. El paquete local con recursos externos tiene 189
archivos y manifiesto SHA-256 verificado. El test en Vita3K sigue pendiente:
la instalación local termina con `0xC0000005` antes de iniciar cualquier
título. El knockback nativo también permanece pendiente hasta mapear el evento
de daño y sus argumentos.

## Auditoría de campaña persistente

Con las fuentes actuales se regeneraron proofs independientes de los tres
pisos para Rogue: Dragon termina con 10/80 HP; Worm termina con 4/80; el piso
final termina con 45/80. Esos proofs empiezan cada uno con 80 HP y 30 flechas.
La prueba encadenada sí transfiere al piso 1 el estado correcto (10 HP y 23
flechas), y muere en el tick 850: recibe 4 y 3 de daño del Worm y 3 del Bat en
37 ticks. La prueba integrada ahora compara clase, HP, HP máximo y flechas del
inicio de cada proof contra el estado cargado, y se detiene antes de simular el
piso 1 cuando detecta el estado incompatible. Una corrida anterior que no
tenía ese gate sí reprodujo la muerte a tick 850. Por tanto no se presenta el
éxito de rutas separadas como una campaña completada.

Una prueba experimental del controlador intentó preservar el tiro durante la
evasión y escoger una línea de arco bajo el saliente. La variante quedó
atascada antes del jefe o murió antes de llegar a su zona; no se incorporó al
VPK ni a la ruta base. La limitación concreta que sigue abierta es una ruta
continua que sobreviva al Dragon y pueda derrotar al Worm con la vida y flechas
transportadas.

Véase [Demo ACT_ONE 02.03](DEMO_ACT_ONE_203.md).
