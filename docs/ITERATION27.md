# Iteración 027: interacción melee con cofres ACT_ONE

El runtime prueba la caja poligonal melee activa contra los cofres VWE2 antes
de avanzar el frame del ataque. Un contacto marca `WORLD_ENTITY_OPENED` una
sola vez; el renderer selecciona el frame abierto y el log registra la
interacción. No se concede loot: el análisis de Ghidra muestra que
`Chest::open` consulta bloqueo/rareza y delega los objetos a `ItemGenerator`,
que todavía no está reconstruido.

El build se produjo con VitaSDK ARM desde Windows, sin CMake ni WSL accesible.
`tools/build_vpk_windows.ps1 -Iteration 027` deja los objetos y el VPK solo en
`_local/build-act-one-027`. Las cuatro fuentes runtime principales compilan
con `-Wall -Wextra -Werror`; los tests de entidades y jugador también compilan
a objetos ARM. La suite Python pasa 101/101. El VPK tiene identidad
`VAGV00006`, `Vagante2Vita`, versión `00.27`, CRC ZIP válido y únicamente
`eboot.bin`, `param.sfo` y los recursos LiveArea esperados.

Se ejecutó el smoke automatizado con la carpeta privada del mundo ACT_ONE en
una filesystem dedicada de Vita3K. El proceso quedó vivo sin ventana visible,
`vita3k.log` permaneció vacío y no se creó un log de sesión nativo; el
recolector registró `passed=false`, razón `No new native session log was
created`. Se detuvo solo ese proceso de prueba. Por tanto, el VPK está
compilado y estructuralmente validado, pero no hay evidencia de ejecución del
juego en Vita3K. Un segundo intento en otra carpeta aislada con ventana visible
dio el mismo resultado. Se observaron dos procesos Vita3K activos bajo el
proyecto Touhou; permanecieron intactos. Un conflicto de instancia única es
posible, pero no está confirmado. Los dos informes privados están bajo
`_local/vita3k-act-one-027/runs/iteration027/run-000001` y
`_local/vita3k-act-one-027-visible/runs/iteration027/run-000001`.

ACT_ONE continúa incompleto: el loot y bloqueo de cofres, activación/daño de
trampas, comportamiento de entidades, generación exacta, transición al piso
siguiente y validación automatizada dentro de Vita3K siguen pendientes. El
objetivo de mundo completo no se considera cumplido.
