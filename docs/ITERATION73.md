# Iteración 073: sprites de enemigos y paquete ACT_ONE

El render de ACT_ONE ya usa las hojas originales de Goblin y Bat, que son los
enemigos del primer piso exportado, con estados básicos de reposo, movimiento,
aire y daño. El bundle también prepara las hojas compatibles de Lurker y Slime
para los pisos posteriores. Los sprites comerciales permanecen en el paquete
externo privado; el VPK solo contiene código propio. La IA, ataques, daño,
animaciones exactas y sus sorteos siguen provisionales.

Al preparar la prueba detecté que el bundle manual 072 no tenía `item-loot.vlt`
ni los atlas de objetos aunque el runtime los exige al iniciar ACT_ONE. 073
regenera el paquete desde las herramientas reproducibles, incluye la tabla VLT1
y sus siete atlas, los archivos de terreno y entidades de los tres pisos, y las
hojas de enemigos. `_local/delivery-act-one-campaign073` valida 94 archivos;
el test aislado copia también el directorio `enemies` a Vita3K.

El VPK se compiló con VitaSDK en
`_local/build-act-one-073/Vagante2Vita-iteration073.vpk` (827,216 bytes), SHA-256
`68DC7533312DBC6A27445E3311EB257B14CAF24F809150149FC306C9FC3752AD`. Contiene
las siete entradas VPK esperadas. La suite Python pasa 140 pruebas y
`world_visuals.c`, `world_entities_test.c` y `player_test.c` compilan para ARM
con `-Wall -Wextra -Werror`.

El smoke de salida ACT_ONE usó ese VPK y el bundle 073, preparó una replay de 34
pasos para una ruta de 56 tiles y agotó 45 segundos. Vita3K terminó con código
`-1` sin crear sesión nativa; `stdout.log`, `stderr.log`, `vita3k.log` y los
logs de depuración quedaron vacíos. Por eso la carga del juego, los sprites y la
transición entre pisos siguen sin validación dentro del emulador. El primer
mundo no se declara completo: la generación no tiene paridad de seed y enemigos,
varias trampas y otras reglas conservan modelos provisionales.
