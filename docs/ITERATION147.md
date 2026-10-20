# Iteración 01.47: movimiento y contacto provisional del Dragon

El Dragon VWE3 ahora persigue al jugador en el aire con velocidad limitada a
1.5 px/tick y barrido contra tiles, y el host modela su contacto como 5 de daño
con 30 ticks de enfriamiento. Los valores de cuerpo 13×13, 65 HP y daño base 5
vienen de constructores/stats exportados por Ghidra; la persecución y cooldown
son una integración provisional, no una afirmación de paridad de IA nativa.
Las pruebas C verifican movimiento, hitbox, daño y enfriamiento. Worm queda
fuera del combate genérico hasta implementar su cabeza y 20 crawlers enlazados.

Build VitaSDK Windows: `_local/build-act-one-147/Vagante2Vita-iteration147.vpk`,
ID estable `VAGV00006`, versión `01.47`. SHA-256:
`E279ED36FEE446428362A31C4DC6AC7CF6A8D5967518020523BB440D550F7AB2`.
Validación: 13 pruebas C host, 203 pruebas Python y 70 subtests. Los tres
route proofs se regeneraron con la huella
`df377fd364c032b3cb121a45747e84f143a18bcee91b93ead96006a81d6f7de9`; la
entrega local está en `_local/analysis147/campaign-delivery/`.

El smoke Vita3K fue ejecutado desde una copia completa nueva del perfil
Vagante, aislada en `_local/vita3k-complete-test-147/`; el runner verificó los
2,126 archivos por hash, validó el bundle de 142 archivos y preparó 22 pasos /
386 ticks de replay de salida del piso 0. Vita3K agotó 60 segundos antes de
crear proceso de juego/log de sesión (`failure_stage=before_native_session`,
`process_exit_code=-1`, Vulkan/SDL). No constituye validación de ejecución del
VPK. Resultado en
`_local/vita3k-complete-test-147/runs/iteration147/run-000001/result.json`.
La campaña completa tampoco está comprobada: los recursos actuales solo
incluyen al Goblin King, faltan ataques/animaciones nativas de Dragon y Worm
no tiene su cadena de 20 segmentos.
