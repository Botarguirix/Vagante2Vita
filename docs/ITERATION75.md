# Iteración 075: ataque de Goblin desde el script original

El Goblin ya no depende de daño por contacto. La extracción local de `attacks.json`
convierte el script `goblin-stab` a un paquete privado VGA1: conserva sus cinco
keyframes, duraciones `[9, 1, 1, 6, 1]`, polígonos activos, el aviso de 16 frames,
el enfriamiento de 40 frames y daño base de 5–9 observado en Ghidra. El runtime
orienta al Goblin hacia el jugador, muestra la hoja original de ataque y aplica
un solo impacto cuando el polígono activo cruza la caja del jugador. La
multiplicación de daño se limita al rango del script y se redondea hacia arriba.

El RNG local hace el comportamiento reproducible, pero no comparte el flujo MT
global del juego; las tiradas no tienen paridad exacta con una partida original.
La IA actual busca al jugador dentro de un rango limitado. Otros enemigos,
variantes de ataque, knockback y detalles completos de navegación siguen
pendientes.

El paquete privado ACT_ONE se regeneró desde los recursos locales y contiene
99 archivos verificados por tamaño y SHA-256. Mantiene los pisos 1–2 y sus
informes de geometría; sus reparaciones de ruta siguen siendo provisionales.
No contiene los recursos comerciales en el VPK ni en Git.

Verificación: la suite Python pasa 142/142. `world_combat.c`, `world_entities.c`,
`world_visuals.c`, `main.c` y pruebas C relacionadas compilan para ARM con
`-Wall -Wextra -Werror`. El VPK 00.75 se compiló con VitaSDK. La verificación
estructural confirmó las siete entradas permitidas, TITLE_ID `VAGV00006` y
APP_VER `00.75`. El archivo mide 830,131 bytes y su SHA-256 es
`ba405d5a304d794e54114d5b7e872d22fddbfa50d3abbd4cff90370355e5ee8b`.

El smoke automatizado de Vita3K agotó 45 segundos (`exitCode=-1`, `timedOut=true`) antes de que el
emulador generara logs o abriera una sesión nativa. `stdout.log`, `stderr.log`, `vita3k.log` y los logs de depuración quedaron vacíos. El VPK no quedó validado en
emulador. ACT_ONE tampoco tiene paridad exacta de generación ni una ruta física
completa probada dentro del runtime, así que el primer mundo sigue incompleto.
