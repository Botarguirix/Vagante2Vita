# Iteración 090: orden nativo de spawns ACT_ONE

Ghidra muestra que `LevelGenerator::addRandomEnemies` consume un draw uniforme
de 31 bits al entrar, construye las salas en orden x-major y baraja la lista
global; para cada sala elegible crea coordenadas x-major de toda la habitación
y vuelve a barajarlas antes de probarlas. El roll 0–99 también usa
`uniform_int<int>`. `floor_layout.py` ahora sigue esos pasos y recorre las
coordenadas barajadas en secuencia, en vez de escoger cada celda restante con
`raw() % len(candidates)`. El filtro geométrico de Lurker y el presupuesto de
sala de iteración 089 se conservan.

La generación seed 12345 produce 8, 8 y 20 enemigos provisionales. Los
conteos Goblin/Slime/Lurker son 4/4/0, 3/3/2 y 8/9/3 en los pisos 0–2. Los
hashes VWI2/VWE2 siguen coincidiendo con los tres proofs físicos de ruta; solo
cambia VWE3. El bundle privado `_local/delivery-act-one-campaign090` valida
108 archivos, tres pisos y 56 imágenes. Pasan 167 pruebas Python.

VPK 00.90 compilado con VitaSDK Windows: 830,423 bytes, SHA-256
`9A42FCC106F769523AD7B6F5C0AF7ECC9D921CD2518A92C8DDAF21E53F0204A9`. La
replay automatizada conserva 545 segmentos, 3,184 ticks y rutas `player_step`
para los tres pisos. Vita3K 0.2.1.4183 agotó 90 segundos antes de iniciar una
sesión nativa; no generó log, stdout ni stderr. El diagnóstico está en
`_local/vita3k-vagante-test090/runs/iteration090/run-000001/result.json`.

La selección todavía no tiene paridad exacta: los filtros nativos de tiles y
entidades, el RNG intercalado con otros hooks y el orden de entidades de sala
siguen incompletos. Los enemigos permanecen provisionales; jefe, transiciones
validadas en Vita3K/hardware y finalización de la campaña están pendientes.
`floor_complete` y `campaign_complete` permanecen falsos.
