# Tween — rama A

2026-10-24. --unit Tween lista Tween::val @007df280: traducida por completo.
El pseudocódigo perdió las operaciones SSE y devolvía in_ST0; se contrastó toda
la función con llvm-objdump y la tabla de saltos @007df780.

Se recuperaron las doce curvas (linear, quadratic in/in-out, cubic in/out/in-out,
quartic, circular in/out, cosine, bounce y exponential in/out), sus límites y
orden de operaciones. La entrada de quadratic in salta a 007df428: omite el
primer cuadrado del bloque quartic; no se confunden ambas curvas aunque Ghidra
las agrupa. val limita time por arriba a d, no limita por abajo ni protege d=0;
tipo desconocido registra VLOG(LOG_ERROR) y devuelve 0.

Constantes float leídas del PE: pi=3.1415927410125732, bounce 7.5625,
límites 0.3636363744735718/0.7272727489471436/0.9090909361839294,
desplazamientos 0.5454545617103577/0.8181818127632141/0.9545454382896423,
alturas 0.75/0.9375/0.984375; exponentes usan 1024 y 2 con factor -10.

Constructor y step no figuran independientemente en el índice: se recuperan de
sus expansiones en MainMenu::MainMenu/reset/step. El constructor recibe begin,
change,duration y guarda c=change; step consume pauseTime y avanza solo el
sobrante cuando se cruza cero. Los demás helpers sin cuerpo en índice se
conservan como stubs hasta recuperar sus expansiones. Cabeceras intactas.

Verificación: build_recon_pc.ps1 -Run, compilación y cierre normal exit code: 0;
run_recon_tests.ps1 -Unit Tween pasó (12 curvas, extremos, clamp, tramos de
bounce y pause overshoot). Logs _local/menus/Tween_build.log y Tween_test.log;
asm y constantes locales en la misma carpeta. El primer enlace estaba
bloqueado por un proceso previo: se conservó su imagen como
vagante-in-use-21836.exe sin terminar ese proceso. Sin push ni prueba Vita.

Cruce adicional: Client::Client @00549e31/3b escribe b=600 y c=-400; el tercer argumento es el desplazamiento. MainMenu::reset calcula target-b explícitamente. setEndWaitTime asigna el campo mediante el accessor público, como el inline de Client. No se edita Client.cpp.
La corrección del constructor y setEndWaitTime se verificó de nuevo en la copia aislada de la siguiente unidad: _local/menus/Tween_recheck.log. El build de esa copia cerró con exit code: 0; Client.cpp en curso quedó fuera y conserva su stub en esa comprobación.
