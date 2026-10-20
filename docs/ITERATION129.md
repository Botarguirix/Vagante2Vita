# Iteración 129: animación de ataque del Thief

La iteración anterior implementó el golpe aproximado del Thief. Ahora el
renderer selecciona `enemies/thief/attack.png` mientras dura ese golpe, recorre
sus ocho celdas y usa la orientación fijada hacia el jugador. El asset privado
ya estaba dentro del bundle; esta integración hace visible el estado de combate
en ACT_ONE. La duración del golpe y su caja de impacto siguen siendo
aproximaciones, como se detalla en [ITERATION128.md](ITERATION128.md).

## Validación

- `tools/run_host_tests_windows.ps1`: 12/12 pasan.
- `python -m unittest discover -s tests -v`: 194/194 pasan.
- VPK 1.29: `_local/build-act-one-129/Vagante2Vita-iteration129.vpk`,
  837,292 bytes, SHA-256
  `74ec65e6824bd5536f4f1d43c4d4d8455a41c6d247e74cfeafc59517a9f0de7e`.
  Identidad `VAGV00006` / `Vagante2Vita`, versión `01.29`; CRC ZIP correcto y
  siete entradas permitidas.
- Vita3K 0.2.1.4190 volvió a expirar tras 45 segundos antes de una sesión nativa.
  El log del emulador sigue vacío; el reporte del proceso está en
  `_local/vita3k-act-one-127/runs/iteration129/run-000001/result.json`. Esto no
  confirma el render ni el gameplay en emulador.
