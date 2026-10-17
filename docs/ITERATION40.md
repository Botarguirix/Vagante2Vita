# Iteración 040: hitbox recuperado para dardos de DartTrap

El proyectil de `DartTrap` usa ahora una caja de colisión de 6×6 px, el tamaño
confirmado para `Projectile` en la recuperación de Ghidra. El barrido contra
tiles sólidos examina todos los tiles ocupados por la caja del siguiente tick;
el contacto con jugador y límites de nivel usa la misma dimensión. Se añadió
un caso determinista C que comprueba el spawn del dardo, que no golpea antes de
solapar la caja del jugador, el golpe al solaparse y la recarga de 130 ticks.

La reconstrucción sigue aproximada en varios puntos: el disparador de la trampa
no reproduce todas las guardas de movimiento, agua y espacio libre del juego
original; tampoco se modelan todavía todas las reglas de faction y colisión por
actor. El daño físico recuperado está conectado al estado de salud del jugador,
pero las inmunidades y filtros de daño del motor original requieren más
recuperación.

## Verificación

- El build Windows de VitaSDK produjo
  `_local/build-act-one-040/Vagante2Vita-iteration040.vpk`: 808685 bytes,
  SHA-256 `885E845773379FE349EAE6A9875459353DC4CE08C1DF6EAC8F653CF8650FDE75`.
- `arm-vita-eabi-gcc -std=c11 -Wall -Wextra -Werror -UNDEBUG -fsyntax-only`
  pasó para `vita/src/world_combat.c` y `tests/world_entities_test.c`; el build
  completo compiló y empaquetó todos los módulos.
- La suite Python pasó: 119 tests.
- El smoke con la entrega ACT_ONE 047 (SDL/Vulkan, título previamente extraído)
  agotó 45 s. Se añadió una segunda ruta que usa la entrega actual 049 y entrega
  el VPK como argumento de contenido, con SDL/OpenGL; también agotó 60 s sin
  instalar `VAGV00006`, crear `vita3k.log` ni producir log nativo. El proceso
  termina o queda pendiente antes de iniciar el runtime del juego.
- Una ejecución visible mostró el diálogo de Windows `Vita3K.exe - Error de la
  aplicación`: lectura nula en `0x0000000000000000`. Una ejecución anterior de
  la misma versión 0.2.1.4183 registró código `0xC0000005`. OpenGL y Vulkan
  fallaron de forma similar en los intentos históricos; no se atribuye esta
  falla al VPK ni se considera validación de gameplay.
- El harness ahora valida/instala paquetes de raíz como la entrega 049 y tiene
  `-InstallVpkViaCli` para pasar el VPK por la ruta de contenido. Vita3K
  documenta el argumento `.vpk` para instalarlo; el resultado local muestra
  que este ejecutable sigue sin completar esa ruta.
- El test C de runtime no se ejecutó en host: no hay compilador host disponible
  en PATH y `wsl.exe --status` devuelve `Wsl/EnumerateDistros/Service/E_ACCESSDENIED`.

El primer mundo ACT_ONE permanece incompleto: falta la ruta de smoke nativa
verificada dentro de Vita3K, contenido y transiciones de campaña completos y
paridad exacta de generación/combate.
