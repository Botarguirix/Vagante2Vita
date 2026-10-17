# Validación 097: proofs actuales y replay de campaña

La actualización 096 añadió al runtime la regla recuperada de
`Spider::doCollision` y cambió el fingerprint de fuentes del simulador. Se
regeneraron en Ubuntu/WSL las trazas VTR1 y proofs v3 para las rutas de los
tres pisos ACT_ONE, con seed 12345. El probe nativo llegó a las tres salidas:
56 tiles/1,741 ticks/14 saltos, 64 tiles/736 ticks/13 saltos y 39 tiles/375
ticks. Los proofs registran el fingerprint actual
`f1dab2604eaf908e7e4a24a82a32e8591041c0eb129107229aff6fcf516fdeb8`.

Se preparó una nueva copia privada en
`_local/delivery-act-one-campaign096`; el bundle valida 113 archivos, tres
pisos, cinco familias de enemigos y 66 imágenes. `prepare_smoke_replay.py`
acepta las pruebas actuales y produce una secuencia de campaña de 545
segmentos y 3,184 ticks, con fuente `player_step` en cada piso. El manifiesto
y los hashes de los proofs/trazas se validaron después de actualizarse.

Vita3K 0.2.1.4183 repite el fallo `0xC0000005` antes de crear una sesión
nativa tanto en el smoke del primer piso como en el replay de campaña. El
harness aislado no encuentra `vita3k.log` ni log de sesión del juego. El replay
preparado y los recorridos del probe prueban la ruta física del simulador; no
prueban las transiciones en el emulador. `campaign_complete` y
`native_exit_transition_verified` permanecen falsos. No se generó otro VPK:
el código de la aplicación sigue siendo 00.96.

Se ejecutó CMake 4.2.3 con GCC 15.2.0 en Ubuntu/WSL y CTest pasó 12/12. El
test `world-entity-packet` ahora cubre el daño 5–10 de Spider cuando está en
salto, el umbral de rapidez y el cooldown de 30 ticks; la ejecución encontró
y corrigió un byte extra en el fixture VWE3 y el escenario restablece la
rapidez tras el cooldown. Las 170 pruebas Python de la suite también pasan.

Quedan pendientes la IA que active el salto de Spider, el ataque de telaraña,
la paridad exacta de generación y una sesión válida de Vita3K que recorra las
tres transiciones de la campaña.
