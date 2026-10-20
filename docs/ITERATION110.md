# Iteración 110: daño de estadísticas del jugador

## Cambio

El daño derivado de atributos ya llega a las colisiones de armas nativas:

- `Player::updateStats` en Ghidra calcula las bases melee y ranged desde STR/DEX.
- `Player::attack` usa `meleeDamage` o `rangeDamage` y entrega el resultado al
  arma durante `Weapon::startSwing`.
- `Weapon::doCollision` suma `wielderDamage` al rango de daño de la forma antes
  de aplicar multiplicadores. En este runtime, `MeleeState.damage_bonus` lleva
  `DerivedStats.melee_base`; los proyectiles suman `DerivedStats.ranged_base`.
- El test C de melee verifica que el bono de portador se agregue al daño de la
  forma sin cambiar su geometría de impacto.

## Límite de fidelidad

La implementación usa el valor base esperado. El original hace tiradas enteras
aleatorias y puede sumar daño de equipo, perks, carga, crítico y multiplicadores
de arma/estado. Los arcos aún no incorporan todos esos modificadores. No se
afirma paridad exacta de daño.

## Validación

- `arm-vita-eabi-gcc -fsyntax-only -Wall -Wextra -Werror` para
  `melee.c`, `player.c` y `world_combat.c`: correcto.
- Suite Python: 171/171.
- Las 12 pruebas C host del proyecto pasan con el nuevo runner
  `tools/run_host_tests_windows.ps1`; reporte:
  `_local/host-tests110-final/report.json`. CTest no se ejecutó porque no hay
  CMake host disponible.
- Las 3 pruebas de ruta se recompilaron y corrieron con las fuentes actuales,
  seed 12345 y salida v3 con trazas: piso 0, 1,741 ticks; piso 1, 736; piso 2,
  375. El fingerprint del simulador es
  `05d7be9fdc9ec677dd8d62e9144ab2d02970dbb8e04e7bdafd943cfe86206034`.
  Proofs/trazas/logs: `_local/route-proofs110-clang/`.
- El paquete de campaña nuevo valida 113 archivos y el harness acepta los tres
  proofs actuales. Generó una replay ACT_ONE de 545 segmentos/3,184 ticks y
  159 tiles de ruta: `_local/delivery-act-one-campaign110/`.
- WSL aún devuelve `E_ACCESSDENIED`. MSYS2 GCC y su `cc1` se bloquean en esta
  sesión; se evitó esa limitación con LLVM ya instalado, configurado para usar
  las cabeceras/librerías UCRT de MSYS2. Clang sin esa configuración no trae CRT.
- El intento de campaña primero fue rechazado por el harness porque los route
  proofs 109 tenían un fingerprint anterior; se reemplazaron por los proofs
  110 y el harness aceptó la replay actualizada.
- El smoke de introducción sí lanzó Vita3K 0.2.1 desde un perfil aislado y
  preparó una replay de 52 segmentos/1,321 ticks, pero
  el proceso expiró a los 60 segundos antes de crear una sesión nativa. El
  resultado está en
  `_local/vita3k-vagante-test110/runs/iteration110/run-000002/result.json`.
  Esto no valida comportamiento del VPK.
- La replay de los tres pisos lanzó Vita3K desde el mismo perfil dedicado, pero
  volvió a expirar a los 120 s antes de sesión nativa; el resultado queda en
  `_local/vita3k-vagante-test110/runs/iteration110/run-000003/result.json`.
- Se repitió en Vita3K 0.2.1.4183 (OpenGL/SDL, intro, 60 s), 0.2.1.4183
  (OpenGL/consola, intro, 45 s) y 0.2.1.4170 (OpenGL/consola, intro, 45 s);
  todas las pruebas quedaron en `before_native_session`. Después se descargó
  desde el enlace Windows Nightlies oficial una copia independiente 0.2.1.4190
  (ZIP SHA-256 `3F924F408E5AF997784C44C15C12875712D2F18F472E3714834322783442D651`,
  EXE SHA-256 `A576AC696B2FDE9287B743A2AEF0C4A2BB04053CD57A41216FDADDA2EE599A56`)
  y se probó con perfil local nuevo; OpenGL/SDL también expiró a 60 s antes de
  la sesión nativa. Logs vacíos y resultados en
  `_local/vita3k-vagante-test110-latest/runs/iteration110/run-000001/`.
  Los tres paquetes oficiales (4170/4183/4190) no acreditan todavía el arranque
  del VPK. No se usó ni modificó la instancia compartida de `G:\Vita3k`.

## Alcance restante

ACT_ONE aún carece de paridad completa de generación, sistemas de combate,
inventario, jefe/transiciones y validación end-to-end en Vita3K y consola.
