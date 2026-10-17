# Iteración 070: mapa de blockers G/H en endpoints ACT_ONE

`LevelGenerator::addStartAndGoal` (`0x00688e30`) recorre las entidades creadas
por las habitaciones y registra en un set las posiciones de `TYPE_BOULDER_TRAP`
(`'G'`) y `TYPE_BOULDER` (`'H'`). La entrada conserva el primer candidato
barajado que no aparezca en ese set; la meta hace el mismo filtro después de
ordenar y omite además los puntos sumergidos.

El generador Python ahora redondea las coordenadas de entidad antes de
cuantizarlas a tiles, registra trampas y Boulders colocados, y reproduce el
Boulder que `BoulderTrap::setLevel` crea en la primera celda sólida sobre la
columna central de la trampa. El reporte exporta las posiciones, procedencia,
candidatos rechazados y el modelo de coordenadas. Un fixture comprueba el
redondeo y el Boulder vinculado. El barrido de 20 semillas por cada uno de los
tres pisos confirma el filtro en 13 combinaciones; ninguna entrada/meta
seleccionada como candidata comparte tile con un blocker.

La suite Python completa pasa 140 pruebas. En seed 12345/floor0 se registran
dos blockers, pero no cambian las coordenadas de entrada `[34,6]` y meta
`[38,29]`; el VWI2 permanece con SHA-256
`b410f43eb5b429c339643715f28d1ebb3f1d1f60803aaf77803d90bb6d9edcbe`. No se
modificó el runtime C ni se compiló VPK: el export privado está en
`_local/generation073/map-export` y 00.65 sigue siendo el último paquete.
Los draws de algunos hooks de entidades y el desempate de `std::sort` siguen
pendientes, así que la selección no tiene paridad completa por semilla. El
route probe de física continúa sin compilarse por falta de compilador host y
WSL bloqueado con `E_ACCESSDENIED`; gameplay en Vita3K sigue pendiente.
