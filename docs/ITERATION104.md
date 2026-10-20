# Iteración 104: carga y embestida normal del Slime; VPK 01.04

Ghidra `Slime::Slime @ 0x0078f0c0`, `Slime::step @ 0x0078fab0` y
`Slime::doCollision @ 0x00791780` muestran para el comportamiento normal:
ventana horizontal menor de 50 px, diferencia vertical cercana a cero, una
tirada de 1/20 cuando `jumpTimer > 30`, preparación de 60 ticks, embestida a
rapidez 7 por 25 ticks, enfriamiento de 150 ticks y daño de 3–5. El runtime
aplica ese ciclo, limita el golpe a uno por embestida y conserva la colisión
con tiles. El tipo de Slime y el estado de salto periódico no están presentes
en el paquete VWE3; por eso se representa solo la variante normal y la semilla
de la tirada no pretende reproducir el RNG original.

El test de entidades verifica la carga, el impulso, daño, golpe único y
enfriamiento. Build host en WSL y CTest pasaron 12/12. VitaSDK compiló
`_local/build-vita-104/Vagante2Vita-iteration104.vpk` (833,763 bytes), title
ID `VAGV00006`, versión `01.04`, SHA-256
`52CD9C09CF38BD79FFAED3F7A4C706117A79EAD4F6413EA55F204167EB9E24AE`.

Se regeneraron proofs de ruta v3 con fingerprint
`a3e8c79863b0a22163b5c6f4e54194a0209cc062f30d6c30d4a71dcfeb93caca`; los
tres pisos alcanzan la salida en 1,741, 736 y 375 ticks. La entrega privada
`_local/delivery-act-one-campaign104` valida 113 archivos; la replay tiene 545
segmentos, 3,184 ticks y usa las trazas `player_step` de los tres pisos.

El smoke de campaña con VPK 01.04 valida el bundle y prepara la replay, pero
Vita3K 0.2.1 termina con `0xC0000005` antes de crear una sesión nativa. Se
probó Vulkan/SDL con un perfil aislado cuya ruta usa alias cortos de Windows;
el resultado no cambió. El registro está bajo
`_local/vita3k-vagante-test104-short/runs/iteration104/run-000001/`. No hay
prueba de que el juego haya cargado este VPK ni de que completara un piso.
La paridad completa de variantes y saltos del Slime, la transición de campaña
en runtime y la validación en Vita3K o hardware siguen pendientes; ACT_ONE no
está completo.
