# Iteración 01.49: ciclo de proyectil del Dragon

El host modela el ciclo de escupir fuego del Dragon: muestra
`launching_fireballs.png` (cinco celdas de 48×48, constructor Ghidra), espera
55 ticks, crea una bola de fuego de velocidad normal 1.2 px/tick y le da 180
ticks de vida. El proyectil se detiene con terreno y aplica una sola vez 5 de
daño al jugador. La cadencia y el daño son provisionales; Ghidra confirma el
método `shootFireballsAtPlayer`, el atlas, el tamaño de celda y los factores de
velocidad 1.2/1.8125, pero los filtros de acción, RNG y tipo/daño exactos del
proyectil requieren más recuperación. Prueba host: aviso, lanzamiento, impacto
y consumo del proyectil.

VPK VitaSDK Windows: `_local/build-act-one-149/Vagante2Vita-iteration149.vpk`,
ID `VAGV00006`, versión `01.49`. SHA-256:
`1ADDAE17BBE550434FA97E41D4A034084EE900291A356ADB6338ED27A6E5EA8F`.
Validación: 13 pruebas C host, 203 pruebas Python y 70 subtests. Los route
proofs se regeneraron con huella
`779c3ab467553c33451b01b8ef54a797c5508a102a1800afd37ec6b7aa3fe4e8`; bundle
local en `_local/analysis149/campaign-delivery/`.

Limitación de integración: el bundle solo contiene Goblin King en el tercer
piso. Ningún mapa entrega un registro Dragon, así que su IA y proyectil no se
activan durante la campaña actual. Vita3K tampoco validó el arranque en la
iteración 01.47: expiró antes de la sesión nativa incluso con copia completa
verificada de 2,126 archivos. No se afirma validación Vita3K de esta build ni
campaña ACT_ONE completa.
