# Revalidación 095: arranque de Vita3K y análisis aislado de Spider

Se volvió a preparar y validar el bundle privado 094: 113 archivos, tres pisos,
cinco familias de enemigos y 66 imágenes. La replay física de campaña conserva
545 segmentos y 3,184 ticks. No cambió el código del juego ni se reconstruyó el
VPK: el más reciente sigue siendo 00.93, SHA-256
`8BF2E7A27E3DCBD940E67846A175B7D33C3267758846955C670E6988B256C4FA`.

Se probaron varias rutas aisladas con ese VPK y los recursos 094:

- Vita3K 0.2.1.4183, OpenGL y Qt oculto: timeout de 90 s antes de la sesión.
- Vita3K 0.2.1.4183, OpenGL y Qt visible: timeout de 180 s, sin sesión.
- Vita3K 0.2.1.4183, modo consola con VPK: `0xC0000005` antes de la sesión.
- Vita3K 0.2.1.4170, Qt visible y SDL visible: `0xC0000005` antes de la sesión.

El harness aislado dejó `vita3k.log`, stdout y stderr vacíos en esos intentos.
Por tanto, el error sucede antes de que exista evidencia de carga de Vagante;
no se atribuye al VPK ni a una rutina del juego. La validación automática de
manifiesto y recursos sí pasa. La campaña todavía no tiene una reproducción
observada dentro del emulador.

Para investigar Spider sin abrir ni modificar el proyecto Ghidra compartido,
se creó el proyecto temporal `_local/ghidra-spider` y se importaron localmente
el ejecutable y PDB disponibles en `_local/input`. Ghidra 12.1.4 produjo un
decompilado enfocado de `Spider::runAI` (`0x007b18a0`). El constructor y la
rutina recuperada confirman caja 10×8, daño 5–10, aviso de 30 frames,
persistencia de objetivo de 600 frames y estados idle, walk, jump y web.
`runAI` también muestra retarget al perder visibilidad, aproximación y retiro,
preparación de saltos, selección probabilística entre salto y telaraña, y la
creación del proyectil de telaraña. `doCollision` limita el daño de contacto al
estado de salto y mantiene un registro por unidad.

El pseudocódigo de Ghidra tiene variables superpuestas y advertencias de tipos
PDB; aunque los estados y varios umbrales son legibles, las fórmulas completas
del salto y la configuración/efecto del proyectil todavía requieren confirmar
con listados y funciones auxiliares. No se añadió una IA aproximada al runtime
ni se declara que Spider esté implementado.

`floor_complete` y `campaign_complete` continúan falsos. El siguiente paso es
resolver por qué ambos clones de Vita3K fallan antes del log nativo y completar
la recuperación de los parámetros de Spider antes de implementar su IA.
