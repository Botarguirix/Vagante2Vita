# Iteración 01.51: comprobantes actuales de ruta para ACT_ONE

El ejecutor de rutas ahora toma los recursos compartidos desde la raíz de la
campaña, acepta que el piso 0 no tenga jefe y exige Goblin King en el piso 2.
El fingerprint de prueba incluye `campaign_flow.c`, igual que el verificador de
comprobantes. Después de derrotar al jefe, el probe calcula la ruta de salida
desde la posición real del jugador. Esto evita que siga el tramo calculado desde
un ancla del jefe que ya no coincide con la posición final del combate.

Los tres recorridos pasaron con el movimiento del jugador ejecutado por el
probe de host: piso 0 (semilla 1, 438 ticks), piso 1 (semilla 49, 890 ticks) y
piso 2 (semilla 4, Goblin King derrotado, 30 HP restantes, salida en 2,480
ticks). Los comprobantes v3 y trazas están ligados por SHA-256 a las fuentes y
paquetes exactos en `_local/analysis150/route-final4/`. El paquete promovido a
`campaign-delivery/` contiene los tres proofs y sus trazas; la versión anterior
se conserva en `_local/analysis150/campaign-delivery-before151/`. El piso 2 se
reexportó con el ancla de jefe [28, 21], la más cercana al camino nativo
verificado entre entrada y salida. El manifiesto valida 142 archivos y
`prepare_smoke_replay.py` genera el replay de campaña de 607 pasos / 4,140
ticks.

La suite Python pasa 204/204 y el runner Windows pasa 13/13 pruebas C host.
Estas rutas prueban el modelo `player_step` y el combate provisional del Goblin
King; no prueban ejecución en Vita3K ni paridad de generación, IA, RNG,
recompensas o transición nativa de piso. WSL sigue devolviendo
`E_ACCESSDENIED`.

Se repitió la campaña con el VPK 01.50 en la copia aislada completa de Vita3K
(2,018 archivos del perfil fuente verificados por SHA-256; perfil separado).
Vita3K expiró a los 120 segundos antes de abrir una ventana o crear un log de
emulación/sesión nativa. El resultado queda en
`_local/vita3k-vagante-fullcopy-rerun-20261017/runs/iteration150/run-000005/`.
También se preparó una segunda copia completa del perfil con el ejecutable
0.2.1.4190 disponible localmente (datos de perfil verificados; ejecutable
SHA-256 `A576AC696B2FDE9287B743A2AEF0C4A2BB04053CD57A41216FDADDA2EE599A56`).
El sondeo `--version` también quedó esperando al usar la configuración privada,
así que no hay una sesión de juego validada con ese runtime. Durante el
diagnóstico se detectó una instancia activa en `G:\Vita3k`; no se cerró ni se
usó para esta campaña.
Corrección de evidencia: los archivos `debug-stdout.log` y `debug-stderr.log`
en la raíz de la copia 4190 contienen salida antigua heredada al clonar el
perfil (sus fechas son del 12 de octubre); incluyen sesiones de otra build y
no pertenecen a `run-000005`. La corrida 01.50 fue lanzada desde la ruta
`_local/vita3k-vagante-fullcopy-rerun-20261017`, no desde la copia 4190. Sus
logs propios (`vita3k.log`, stdout, stderr y debug logs dentro de
`run-000005`) están vacíos y el proceso expiró. Por tanto, los mensajes de
crash observados en los logs antiguos no son evidencia sobre el VPK 01.50.
El bloqueo sigue sin causa identificada; cualquier nuevo smoke debe mantener
la comprobación de ruta/versión del ejecutable y recoger únicamente logs
escritos durante esa corrida.

Por ello, el replay automatizado está preparado, pero la integración en Vita3K
y el cambio de piso siguen sin validarse. El VPK permanece en 01.50; esta
iteración modifica herramientas y datos privados de prueba, no el ejecutable.
