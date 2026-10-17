# Iteración 077: reparación de geometría de pisos y nueva campaña privada

Se comprobó que existen pruebas históricas de `player_step` para los tres
pisos, generadas en la iteración 034 con semilla 12345: piso 1 llegó en 809
ticks y 7 saltos; piso 2 llegó en 401 ticks sin saltos. Sus proofs están ligados
por SHA-256 a los VWI/VWE exactos de la entrega 047.

Esos resultados no probaban los pisos de la campaña 076. Sus VWI diferían de
los de 047 en 38 tiles para el piso 1 y 37 para el piso 2, incluidos tiles de
escalera y plataforma. La causa fue que los layouts 076 se generaron antes de
que `floor_layout.py` aplicara la reparación de apoyos en la ruta.

Regeneré ambos layouts con la fuente actual, semilla 12345, y los exporté a
VWI2/VWE2/VWE3. La reparación provisional añadió 28 plataformas y 8 tiles de
escalera al piso 1, y 25 plataformas y 36 tiles de escalera al piso 2. Los
endpoints quedaron en (13,16)→(55,21) y (37,38)→(33,6). Los hashes VWI son:

- Piso 1: `33364658a0547dd2583281b2d3b0609d9d00e2c116f47d0b75d019815171a6b4`.
- Piso 2: `1e12d441e130852fcca5b78e1721de74297dfb3c49d3a44b67e412680020f0b4`.

La campaña privada `_local/delivery-act-one-campaign077` conserva el piso 0,
añade los dos exports nuevos y valida 100 entradas del manifiesto. Solo el piso
0 lleva proof físico. Los nuevos pisos mantienen
`native_player_route_verified=false`, `floor_complete=false` y la campaña
`campaign_complete=false`; su ruta de tiles no simula la física del jugador.

Intenté ejecutar el probe C que enlaza `player_step` mediante WSL, pero WSL
terminó antes de iniciar Ubuntu con `Wsl/Service/CreateInstance/E_ACCESSDENIED`.
No se ejecutó compilación ni simulación física de los nuevos bytes. La suite
Python completa pasa 144/144 y el manifiesto de la campaña 077 valida, pero
ninguna de esas comprobaciones sustituye la ruta física.

Se repitió el barrido del generador: 20 semillas por cada uno de los tres pisos
(60 combinaciones) pasan dimensiones Vita, spawn despejado y con apoyo, bloqueo
de endpoints por entidades y conectividad de ruta de tiles. La prueba se ejecutó
aparte y terminó OK en 17 segundos. No simula `player_step` ni acredita que las
60 rutas sean físicamente recorribles.

Siguiente paso: ejecutar `tools/run_act_one_route_probe_wsl.sh` con los VWI/VWE
de `_local/delivery-act-one-campaign077/floors/`, registrar proofs ligados a
estos hashes y regenerar la campaña privada. Si un probe se atasca, corregir la
geometría y repetir. El mundo sigue incompleto y sin validación de gameplay en
Vita3K. También se probó el smoke de salida con el VPK 076 y esta campaña: el
harness generó 34 pasos/536 ticks, pero Vita3K agotó 45 segundos antes de crear
una sesión nativa; `vita3k.log`, stdout y stderr quedaron vacíos. El resultado
está en `_local/vita3k-vagante-clone/runs/iteration076/run-000003`.

El recolector ahora normaliza los códigos de proceso a 32 bits: un timeout
terminado por el harness aparece como `0xFFFFFFFF` y el access violation como
`0xC0000005`. `tests/test_vita3k_harness.ps1` comprueba ambos casos y una salida
genérica.

Los proofs de ruta nuevos usan ahora formato v2: además de los hashes del VWI y
VWE, fijan el hash de las fuentes C que compila el probe, sus headers, el script
de build y las herramientas que registran y validan el proof. Si cambia el
simulador, la validación invalida el proof. Los proofs v1 existentes se aceptan
como registros históricos sin huella de código; el proof del piso 0 076 aún es
v1. Las pruebas de registro y preparación de paquetes pasan dentro de la suite.
