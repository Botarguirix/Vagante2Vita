# Iteración 076: primera ruta física ACT_ONE validada

Se ejecutó `act_one_route_probe.c` con GCC desde WSL, enlazando los módulos
reales `world`, `world_entities`, `world_combat`, `loot`, `player`,
`player_stats`, `melee`, `bow` e `input_replay`, con `-Wall -Wextra -Werror`.
El build repetible quedó en `tools/run_act_one_route_probe_wsl.sh`.
La simulación de la semilla 12345 llegó a la salida del piso 0 tras 1,741 ticks,
14 saltos y 56 tiles de ruta. Terminó con `dead=0`, `ground=1` y el waypoint
final 55/55.

`record_act_one_route.py` creó un proof con hashes SHA-256 del `act-one-world.vwi`
y `act-one-entities.vwe`. `prepare_act_one_campaign.py` verifica esos hashes y
registra `native_player_route_verified=true` solo para el piso 0. El bundle
privado contiene 100 archivos y valida su manifiesto. Los pisos 1–2 aún tienen
rutas geométricas provisionales; no se registran como recorridos físicos.
La suite Python pasa 142/142 pruebas.

El VPK 00.76 se compiló con VitaSDK para Windows: 830,127 bytes, SHA-256
`d2a4dbf27f9d6737f5482e04c4815c6748f58edfd08825f3a29b4956c970aff7`. El
probe de ruta sí se compiló y ejecutó con GCC de WSL; VitaSDK Linux no está
instalado en esa distro, así que el VPK no se compiló allí. La comprobación
adicional del entorno WSL quedó sin ejecutar cuando el aprobador automático
alcanzó su límite de uso.

Vita3K se probó con una copia aislada y el bundle actualizado. El modo Qt/OpenGL
agotó 45 segundos (`exitCode=-1`) sin abrir una sesión nativa ni generar logs;
el modo consola terminó antes de cargar el título con `0xC0000005` (access
violation). El harness ahora distingue timeout de ese fallo de proceso y lo
registra como `failure_stage=before_native_session`. El staging sí copió
`act-one-goblin-attack.vga` (296 bytes). Por eso no se afirma que el gameplay
haya sido validado en emulador. Las transiciones entre pisos, paridad exacta de
generación y el resto del mundo siguen pendientes.
