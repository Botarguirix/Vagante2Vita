# Iteración 01.54: visibilidad de línea para el Dragon

El gate de preparación del Dragon ahora exige distancia menor a 240 px y una
línea de visión sin tiles sólidos entre los centros de ambas entidades. Se
implementó en `world_line_of_sight` como traza de tiles para el runtime Vita y
se cubrió el caso despejado, el muro interpuesto y el comienzo del ataque al
retirar el muro. Es una aproximación host del `Entity::testLine` de Ghidra;
los desempates exactos de `drawLine` en esquinas siguen pendientes y el resto
de la IA/selección de acciones Dragon continúa provisional.

El VPK 01.54 compila con VitaSDK. Pasan los 13 grupos C host, las 204 pruebas
Python y la comprobación de sintaxis ARM con `-Wall -Wextra -Werror`. La
campaña de tres pisos se volvió a validar con 142 entradas; sus tres rutas
pasan, incluyendo derrota del Goblin King con 30 HP, y el replay preparado
contiene 607 pasos / 4,140 ticks. El paquete de entrega no se modificó fuera
de la copia privada `_local/campaign-154-input`.

La instancia aislada completa de Vita3K verificó 2,162 archivos e instaló el
VPK, pero el emulador 0.2.1.4183 terminó con `0xC0000005` antes de crear sesión
nativa. Por tanto el juego no llegó a ejecutarse y no hay validación de runtime
en Vita3K. El VPK actual está en
`_local/build-act-one-154/Vagante2Vita-iteration154.vpk` (842,597 bytes,
SHA-256 `7CB45C69FC9B3F4D2FAE8337F3A91A1961DEDE362649A32705DCDC3611865EC0`).

ACT_ONE continúa incompleto: el smoke host no sustituye una campaña ejecutada
en emulador o consola, y el Dragon no tiene aún paridad completa de IA.
