# Iteración 030: colisión descendente con pinchos ACT_ONE

`Spikes::Spikes` activa el objeto cuando el tile bajo él es tipo 11.
`Spikes::doCollision` filtra por velocidad vertical descendente superior a
0.4, comprueba la colisión y envía `DamageEventMeta(999, DAMAGE_PURE, true)`.
El runtime ahora aplica muerte al jugador al solaparse con una entidad VWE2 de
pinchos activa en ese tile mientras cae por encima del umbral. Al morir, el
loop registra `WORLD_DEATH` y vuelve al selector de clase para reintentar.

Se añadieron casos al test C del jugador para caída letal, tile no activo y
ascenso. El test compiló como ELF ARM con VitaSDK 15.2.0 y
`-Wall -Wextra -Werror`; **no se ejecutó** porque aquí no hay compilador host y
el runner ARM local no es funcional. El VPK 00.30 sí se construyó con VitaSDK.
Vita3K 0.2.1.4183 se lanzó en una instancia clonada separada, pero permaneció
sin log nativo de la app; el smoke se registró como fallido y no verifica esta
función.

SHA-256 del VPK:
`562BF2D0471966F4AF68750DEA5B476248B16225C9B23B8950E10DC6DB0D1409`.
No se incluyeron recursos comerciales dentro del VPK. La lógica omite todavía
guardas de inmunidad, agua y estados de resurrección vistas en Ghidra, así que
no es paridad completa. Loot/inventario, enemigos originales, transición de
pisos y recorrido del mundo permanecen pendientes.
