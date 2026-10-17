# Iteración 093: soporte de Spider ACT_ONE en el runtime

La rama ACT_ONE de `LevelGenerator::addRandomEnemies` puede elegir Spider
(ID 14) en pisos 1 y 2. La seed 1 del piso 1 produce un Spider; antes de esta
iteración, `encode_enemies` y el cargador VWE3 rechazaban ese ID, y el
renderizador no tenía una familia visual para él. El exportador y el runtime
ahora aceptan ID 14, usan su caja de 10×8 recuperada de
`Spider::Spider` (`0x007aff60`) y cargan los sprites originales idle, walk,
jump, hurt y webshot desde el delivery privado. El preflight siempre revisa
esos cinco estados, aunque la seed activa no contenga Spider. La prueba de seed
1 verifica el camino completo del generador al paquete VWE3.

La IA, el salto, el disparo de telaraña y el daño de Spider siguen sin estar
implementados: el runtime lo mueve con el controlador provisional compartido
de enemigos. Por tanto, la adición evita fallos de exportación/carga y muestra
el sprite correcto, pero no reclama paridad de combate.

La suite pasa 170 pruebas Python. VitaSDK compila los cuatro archivos C
modificados —runtime y prueba de entidades— para ARM con `-Wall -Wextra
-Werror`. El bundle privado `_local/delivery-act-one-campaign093` valida 113
archivos, tres pisos, cinco familias y 66 imágenes.

El VPK 00.93 mide 830,445 bytes y su SHA-256 es
`8BF2E7A27E3DCBD940E67846A175B7D33C3267758846955C670E6988B256C4FA`. Su SFO
declara `VAGV00006`, `Vagante2Vita`, `00.93` y contiene las siete entradas
permitidas. El replay estándar de Vita3K agotó 90 segundos antes de crear una
sesión nativa; los logs quedaron vacíos. El replay de campaña no se pudo
preparar porque los proofs físicos tienen un fingerprint de fuentes anterior.
WSL respondió `E_ACCESSDENIED` al enumerar distribuciones, así que no se
regeneraron trazas físicas en esta iteración. No se valida gameplay en Vita3K.

`floor_complete` y `campaign_complete` siguen falsos. Faltan la IA y el
combate nativos de Spider, proofs de ruta regenerados con las fuentes actuales,
paridad completa de generación y una ejecución de campaña observada en
Vita3K o hardware.
