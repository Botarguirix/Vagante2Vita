# Iteración 039: contacto de enemigos y causa de muerte

Ghidra recuperó `Bat::doCollision`: cada Bat conserva `lastHurtUnitFrames` y lo
reinicia a 30 después de un golpe válido. El runtime usa ahora un cooldown de
contacto individual de 30 ticks para cada enemigo ACT_ONE provisional. Si el
cuerpo provisional de 12×12 px toca al jugador, causa 1 punto de daño escalado
por el multiplicador base de clase; el valor 1 y el tamaño de contacto son
aproximaciones, porque no se recuperó aquí el cálculo completo de daño de cada
monstruo ni los filtros de colisión de `Monster`.

Las causas de muerte ya se conservan como `spikes`, `dart_trap` o `enemy` en
`NativePlayer` y el log no atribuye todas las muertes a pinchos. La ventana de
30 ticks de los enemigos está separada del cooldown que limita ataques del
jugador contra ellos.

## Verificación

- Se añadieron casos C para daño de contacto, cooldown de 30 ticks, muerte por
  enemigo y causa de muerte por pinchos.
- VitaSDK ARM pasó `-Wall -Wextra -Werror -fsyntax-only` sobre los módulos de
  combate/jugador/render y los tests C de jugador y entidades. El build compila
  además `main.c` con sus macros de versión. Los tests C no se ejecutaron:
  WSL devuelve `E_ACCESSDENIED` y no hay compilador host disponible.
- VPK `VAGV00006`, versión `00.39`, 808661 bytes; SHA-256
  `1FBA9C53E5B2110ED2397684C693DBF07A5E6ED4B4890400FAF15AEBC1123BE8`.
- Smoke ACT_ONE Vita3K (Vulkan/SDL) agotó 30 s y terminó con código -1; no
  apareció ventana, `vita3k.log` ni sesión nativa. Resultado en
  `_local/vita3k-iteration039/runs/iteration039/run-000001/result.json`.

ACT_ONE sigue incompleto: el daño de monstruo aún no coincide con datos nativos,
las animaciones y ataques son provisionales, faltan loot, paridad exacta de
generación y validar transición y campaña en Vita3K funcional.
