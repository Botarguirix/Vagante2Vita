# Iteración 130: ciclo provisional de carga del Goblin King

El registro VWE3 del Goblin King ya ejecuta y muestra una carga. Ghidra
confirma que `chargeAttack` usa un aviso de 35/75 ticks, reduce la velocidad
actual a 70%, luego lanza a ±9.3 px/tick y `step` amortigua el movimiento. El
runtime implementa esa secuencia, una condición local de proximidad Manhattan
<128, barrido horizontal del cuerpo 14×12 contra tiles sólidos y el atlas
`charge_attack` de cuatro cuadros. La selección por proximidad reemplaza
provisionalmente a la cola aleatoria nativa; la polaridad exacta entre flag y
dirección, daño, colisión con jugador, stun, derrota y transición siguen sin
implementarse. El boss aún no se genera automáticamente en los layouts ACT_ONE.
No se declara una pelea completa ni paridad nativa.

## Validación

- `tools/run_host_tests_windows.ps1`: 12/12 pruebas C, incluida la cuenta
  regresiva de 35 ticks, el impulso de 9.3, el desplazamiento, frenado y la ruta
  de 75 ticks en dirección opuesta.
- `python -m unittest discover -s tests -v`: 194/194.
- Los route proofs se regeneraron con el nuevo fingerprint del simulador; seed
  1/49/4 completan las rutas físicas en 438/890/546 ticks. Bundle privado:
  `_local/iteration130/campaign`, 142 archivos validados; replay actualizado:
  `_local/iteration130/campaign-replay.vts`.
- VPK 1.30: `_local/build-act-one-130/Vagante2Vita-iteration130.vpk`,
  837,697 bytes, SHA-256
  `d7c33536b5055700f12ef768fa7106e4a4943534976172dc3e8cce81fd99509e`.
  Identidad `VAGV00006` / `Vagante2Vita`, versión `01.30`; CRC ZIP válido y
  siete entradas permitidas.
- Vita3K 0.2.1.4190 vuelve a agotar 45 s antes de crear el log de sesión
  nativa. Resultado: `_local/vita3k-act-one-127/runs/iteration130/run-000001`.
  No hay validación integrada del VPK en esta ejecución.
