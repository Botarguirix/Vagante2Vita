# Iteración 111: marcador honesto del final de los tres pisos

La ruta de replay existente cubre tres paquetes ACT_ONE y dos cambios de piso,
pero todavía no integra sala ni combate de jefe. `main.c` ya no registra
`ACT_ONE_DUNGEON cleared=1` al salir del tercer piso: ahora emite
`ACT_ONE_FLOORS complete=1 count=3 boss_transition_pending=1 campaign_complete=0`.
El smoke espera ese marcador y reporta explícitamente que completó los tres
pisos sin declarar completa la campaña.

La evidencia local de Ghidra identifica la rama ACT_ONE de `addBoss`
(`0x0068a770`) con `BossGoblinKing` (`0x005157c0`). La base privada de recursos
enumera doce atlas del Goblin King y sonidos de ACT_ONE. Todavía faltan las
reglas de arena/spawn, la integración de esos atlas, la IA/combate del jefe y
la transición posterior a su derrota. Por ello esta iteración no completa el
primer mundo.

## Validación

- `python -m unittest tests.test_vita3k_smoke`: 11/11.
- `python -m unittest discover -s tests -v`: 171/171.
- VitaSDK Windows construyó `_local/build-act-one-111/Vagante2Vita-iteration111.vpk`.
- `tools/package_iteration.py`/SFO: identidad `VAGV00006`, título
  `Vagante2Vita`, versión `01.11`; el VPK contiene siete entradas y pasa la
  comprobación ZIP. Tamaño 835,757 bytes; SHA-256
  `9b1d1bd6be0e7091a9d613791c43c1f37a31fdadce3bbdd8a7ec8d9433991c79`.
- El test automatizado comprueba el marcador de tres pisos y aún falla si falta
  dicho marcador. No se volvió a lanzar Vita3K porque las pruebas 4170, 4183 y
  4190 ya fallaron repetidamente antes de crear sesión nativa; este cambio de
  telemetría no altera el arranque. No hay validación nueva de gameplay en
  Vita3K ni hardware.
