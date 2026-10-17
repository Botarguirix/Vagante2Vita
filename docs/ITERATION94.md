# Recuperación 094: proofs físicos de campaña y replay aislado

Se inició Ubuntu en WSL de forma explícita y se repitió el probe nativo con
las fuentes actuales para ACT_ONE piso 0, piso 1 y piso 2. Las tres rutas
alcanzan su salida: respectivamente 56 tiles/1,741 ticks/14 saltos,
64 tiles/736 ticks/13 saltos y 39 tiles/375 ticks. Cada ejecución generó una
traza VTR1; los proofs v3 quedan vinculados a los hashes del mapa, entidades,
traza y fingerprint actual del simulador.

El paquete privado `_local/delivery-act-one-campaign094` se reconstruyó desde
el delivery 093, conservando sus recursos, y ahora contiene los proofs y
sidecars VTR1 renovados. El manifiesto valida 113 archivos y el preflight
valida tres pisos, las familias Bat, Goblin, Lurker, Slime y Spider, y 66
imágenes. `prepare_smoke_replay.py` produjo el replay de campaña de 545
segmentos y 3,184 ticks con rutas físicas en los tres pisos.

Vita3K 0.2.1.4183 se ejecutó con la instancia local aislada y el bundle 094.
La validación de recursos pasó, pero el proceso agotó 90 segundos antes de
crear una sesión nativa. `vita3k.log`, stdout y stderr quedaron vacíos; por eso
esta ejecución no prueba carga de mundo ni transiciones. Se usó el VPK 00.93,
sin cambios de código: 830,445 bytes, SHA-256
`8BF2E7A27E3DCBD940E67846A175B7D33C3267758846955C670E6988B256C4FA`.

Python 3.12.10 ya estaba instalado en Windows en
`%LOCALAPPDATA%\\Programs\\Python\\Python312\\python.exe`; el comando
`python` resolvía al alias inaccesible de Microsoft Store. La ejecución de
estas herramientas usó explícitamente el Python instalado. No se alteraron
el instalador ni el PATH del usuario.

`floor_complete` y `campaign_complete` siguen falsos. Aún faltan la IA y el
combate nativos de Spider, paridad de generación completa y un replay de
campaña que llegue a crear una sesión en Vita3K o hardware.
