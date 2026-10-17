# Iteración 072: Boulder independiente en ACT_ONE

El runtime ahora ejecuta el Boulder independiente (VWE2 kind 22). La lectura de
`Boulder::step` en Ghidra mostró una espera de sacudida de 120 frames antes de
caer; tras la espera, la implementación inicia la caída con velocidad vertical
0.32 y aplica gravedad de 0.16 por frame hasta aterrizar sobre tiles sólidos.
La caja aproximada de 16×16 coincide con el tamaño usado por el runtime. El
contacto del Boulder con el jugador reutiliza la regla existente de impacto:
daño una vez por entidad cuando supera el umbral de velocidad. Se dibuja
`shake_boulder.png` durante la espera y el sprite normal al caer.

El registro de entidades de sala no expone el subtipo del Boulder. Esta versión
asume el subtipo de sacudida como valor por defecto; la asociación requiere
confirmación con la construcción de entidades en el ejecutable original. No se
reproducen aún la sincronización visual de sacudida, el ángulo y su aleatoriedad,
sonidos, partículas, rumbler ni todas las interacciones de colisión y daño.
Por tanto, el comportamiento es una reconstrucción inicial, no una equivalencia
completa.

La suite Python pasa 140 pruebas. Los tests C nuevos de `world_entities` y
`player` compilan para ARM con VitaSDK y `-Wall -Wextra -Werror`; no se pudieron
ejecutar porque WSL devuelve `E_ACCESSDENIED` y no hay compilador C host. El
VPK se compiló en
`_local/build-act-one-072/Vagante2Vita-iteration072.vpk`, SHA-256
`2EF324F1820148A24299F4A1379D71362BAC4B826B35888BBA6616FC2CDA5A21`. El
paquete privado `_local/delivery-act-one-campaign072` contiene 69 archivos y
pasa la validación del manifiesto.

El smoke automatizado ejecutó ese VPK con ese bundle en el clon aislado de
Vita3K, SDL/Vulkan y un límite de 30 segundos. El proceso terminó por timeout
sin iniciar el emulador: `stdout.log`, `stderr.log`, `vita3k.log` y los logs de
depuración quedaron en cero bytes; no se creó sesión nativa. Así que no se
validó la carga del título ni el gameplay. El primer piso ACT_ONE sigue
incompleto y la generación aún no tiene paridad de seed.
