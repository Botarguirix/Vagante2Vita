# Iteración 034: rutas físicas provisionales de los tres pisos

El probe host ahora calcula una ruta que combina escaleras y enlaces de salto
aceptados solo si una simulación de `player_step` despega, atraviesa el espacio
sin colisión y aterriza en la plataforma destino. El runtime también corrige la
posición horizontal tras una colisión para eliminar solapamientos subpíxel con
el muro. Se añadió una regresión que deja al jugador caer y pasar bajo un muro;
la batería nativa queda en 11/11.

La generación de los pisos 1 y 2 añade apoyos y escaleras a lo largo de la ruta
ortogonal entrada-salida. Es una reparación provisional de geometría, no la
generación original de Vagante ni paridad exacta. En seed 12345, los tres pisos
alcanzaron su salida con `player_step`:

| Piso | Resultado | Pasos de ruta | Saltos |
| --- | ---: | ---: | ---: |
| Base | 664 ticks | 59 | 3 |
| 1 | 809 ticks | 73 | 7 |
| 2 | 401 ticks | 40 | 0 |

Los informes en `_local/generation046/floor-*-route-proof.json` incluyen hashes
SHA-256 de los VWI/VWE exactos. La entrega privada `_local/act-one-campaign047`
valida 68 archivos; su `campaign.json` reconoce las tres rutas como verificadas
y conserva `floor_complete=false`, `native_exit_transition_verified=false` y
`campaign_complete=false`.

Se compiló con VitaSDK en WSL
`_local/build-act-one-033-wsl/Vagante2Vita-iteration033.vpk`, identidad
`VAGV00006` / `00.33`, siete entradas permitidas y SHA-256
`d8577b45925952fa62e2ad20393160faef2387062aab24af7c4264dad0b4b183`.
Las 117 pruebas Python pasan.

Vita3K 0.2.1.4183 no creó sesión nativa en dos intentos aislados con este VPK y
esta campaña: Qt/Vulkan y SDL/OpenGL agotaron sus límites (60 s y 45 s), sin
log nativo ni `vita3k.log`. Por tanto, no se ha probado que el VPK arranque, que
cargue la campaña instalada ni que el juego cambie de piso. El bundle no incluye
recursos comerciales en el VPK.

Pendientes: resolver el arranque/sesión de Vita3K, probar las dos transiciones
en ejecución, implementar comportamiento completo de enemigos/entidades y jefe,
y reconstruir la generación con paridad exacta. Las rutas actuales demuestran
recorribilidad de una semilla con geometría provisional; todavía no equivalen
a un primer mundo completo de Vagante.
