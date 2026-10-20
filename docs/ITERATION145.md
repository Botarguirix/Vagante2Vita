# Iteración 01.45: hitbox del Dragon en combate cuerpo a cuerpo

El runtime conservaba el tamaño 14×12 del Goblin King al resolver los golpes
contra cualquier jefe. Ahora obtiene el rectángulo por subtipo, por lo que
BossDragon usa su cuerpo 13×13 confirmado por el constructor de Ghidra. Se
agregó una prueba host que golpea solamente la franja inferior adicional del
Dragon; fallaba con el rectángulo anterior y ahora reduce correctamente sus HP.

Validación: 13 pruebas C pasan; 202 pruebas Python y 70 subtests pasan. Se
regeneraron los proofs y trazas con la huella `af7dc80c3f1a5a3e32750abe85c7f9491ab8a1c6b25b3c312e248b316588caed`;
los tres pisos pasan y la traza del piso 2 derrota al Goblin King con 41 HP.
La entrega `_local/analysis145/campaign-delivery/` mantiene tres rutas y
`campaign_complete=false`; Dragon continúa sin IA/ataques integrados y Worm
sigue sin representación de sus 20 segmentos enlazados.

VPK VitaSDK Windows: `_local/build-act-one-145/Vagante2Vita-iteration145.vpk`,
identidad `VAGV00006`, versión `01.45`, siete entradas permitidas. SHA-256 del
VPK `3459AE3D3141B6FC4B827DD0CF2AF9E55A041FBCF85C13F4293AFAB7407B9425`;
SHA-256 de `eboot.bin` `C862AB333AFF96D170610901150D18478C4DCF550045D3982F7AF56AC285D39A`.

Vita3K volvió a agotar 30 s antes de crear sesión nativa. El runner verificó
por SHA-256 la copia completa de 2,126 archivos, validó los 142 archivos de la
campaña y preparó el replay de 494 pasos / 3,821 ticks. No hubo log de Vita3K;
este resultado no valida el gameplay. Evidencia:
`_local/vita3k-vpk145-fullcopy-test/runs/iteration145/run-000001/`.
