# Iteración 087: presupuesto de enemigos ACT_ONE

Se recuperó la rama ACT_ONE de `Room::monstersAllowed` desde la desensamblación
x86 del ejecutable privado porque el pseudocódigo exportado por Ghidra solo
contenía el stub de retorno XMM. El PDB confirma que `Room::act` está en
`+0x80`; la rutina devuelve `(floorIndex >> 1) + 0.5` y añade `0.5` en New Game
Plus. `LevelGenerator::addRandomEnemies` llama con los datos actuales del
nivel y tercer parámetro `false`.

Los pesos de spawn ACT_ONE son 1.0 para Goblin, Bat, Lurker y Slime. Por eso,
el tope derivado por sala es uno en pisos 0 y 1 y dos en el piso 2. El modelo
provisional ahora usa ese límite y evita reutilizar una misma celda para los
dos spawns. Con seed 12345, la entrega privada
`_local/delivery-act-one-campaign087` genera 8, 8 y 20 spawns provisionales en
los tres pisos.

Los archivos VWI2 y VWE2 de los pisos 1 y 2 son byte por byte iguales a los de
la entrega anterior, por lo que sus proofs de ruta física siguen ligados a los
mismos mapas y entidades. Solo cambió el paquete VWE3 separado de enemigos. El
bundle valida 108 archivos; el preflight valida tres pisos, las familias de
enemigos disponibles y 56 imágenes. La suite Python pasa 164 pruebas y 66
subpruebas; los 12 tests C host de la iteración 086 pasan y el VPK 00.86 fue
compilado con VitaSDK Windows.

La selección y el orden de celdas, filtros de cada superficie, peso de sala y
consumo RNG total aún no tienen paridad completa. No se afirma que los 20
spawns del piso 2 coincidan con el juego original; `floor_complete` y
`campaign_complete` siguen falsos. El intento de replay automatizada con el
bundle 087 y VPK 00.86 terminó a los 60 segundos antes de crear una sesión
nativa en Vita3K (`iteration086/run-000002`), sin ventana ni log del emulador.
