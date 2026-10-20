# Iteración 01.36: ruta física con derrota del Goblin King provisional

El probe de ruta de campaña ahora compone una ruta desde la entrada al Goblin
King, inicia combate cuerpo a cuerpo, espera a que el jefe sea derrotado y
continúa hasta la salida. Durante el combate no interpreta la inmovilidad
esperada del jugador como un bloqueo; también salta si el jugador queda por
debajo del cuerpo del jefe y necesita alinear el ataque.

La ubicación inicial del actor (`21,20`) no tenía rutas transitables desde la
entrada. El exportador ahora prefiere el ancla candidata más cercana a la ruta
de tiles del piso; el paquete coloca al jefe en `(28,21)`, la cuarta geometría
documentada para el piso 2. Con el seed 4, la simulación host lo derrota (70 a
0 HP), termina con 41 HP y llega a la salida en 2,161 ticks. El paquete conserva
`native_spawn_ready=false`, el
`native_parity=false` del actor y `campaign_complete=false`: la ubicación es
una reparación de prueba provisional, no una afirmación de equivalencia con
la generación del juego original.

La validación posterior detectó que el exportador escribía el origen del tile
(`[448,336]`) como origen del cuerpo del jefe, mientras el paquete de campaña
usaba el centro (`[456,344]`). El origen fallaba la ruta física después del
combate. El exportador ahora escribe el centro del tile (`[456,344]`) y las pruebas
comprueban ambas coordenadas. Reexportar el piso 2 y volver a ejecutar el probe
reproduce la derrota, los 41 HP restantes y la salida en 2,161 ticks. Las tres
trazas y proofs se regeneraron en `_local/analysis143/routes/`; el resumen marca
los pisos 0, 1 y 2 como pasados y el paquete resultante está en
`_local/analysis143/campaign-delivery/`.

Los tres pisos tienen proofs y trazas VTR1 ligados al hash actual del
simulador. El runner Windows compila el probe con Clang/UCRT, ejecuta los tres
pisos y calcula la huella que consume el empaquetador; también comprueba que
las fuentes no cambien entre compilación y registro del proof. El replay de
campaña consume `player_step:0`, `player_step:1` y `player_step:2`, y marca 494
pasos / 3,821 ticks nominales. El proof del piso 2 registra derrota, vida
restante, ticks de combate y waypoint final. La entrega actualizada valida 142
archivos. Resultado reproducible en
`_local/analysis143/routes/route-results.json`; paquete de prueba en
`_local/analysis143/campaign-delivery/`, y replay en
`_local/analysis143/campaign-replay.vts`.

Validación actual: las 201 pruebas Python pasan con `unittest discover -s
tests -p test_*.py`, y también pasan las 12 pruebas C host de Clang/UCRT. El
probe se recompiló y completó los tres pisos. No se pudo instalar pytest por
las restricciones de red de Windows y el servicio WSL respondió
`E_ACCESSDENIED`; `unittest` ejecutó el conjunto completo. Se limitó el
descubrimiento predeterminado de pytest a
`tests/` para excluir suites de PyGhidra vendorizadas dentro de `_local`. La
sonda C se compiló con Clang, `-Wall -Wextra -Werror`. El runtime también
bloquea la salida del piso 2 si falta el jefe o queda alguno vivo; el smoke
exige el marcador de derrota antes de la finalización de campaña.

El VPK 01.36 se compiló con VitaSDK estricto. Identidad SFO: `VAGV00006`,
`Vagante2Vita`, versión `01.36`; 7 entradas, `eboot.bin` de 191,985 bytes,
838,890 bytes totales. SHA-256:
`7E149DEE983BBD1FA06C8F10FB0B87E7A7EC3C34351B6300924579939DA2DA3E`.
Artefacto: `_local/build-act-one-136/Vagante2Vita-iteration136.vpk`.

## Vita3K

Se clonó la instancia completa local de 0.2.1.4190 y se corrigió el error que
había pasado inadvertido en los clones anteriores: `runtime/config.yml`
apuntaba a un filesystem de otra prueba. El nuevo clon tiene su propio
`pref-path` dentro de `_local/vita3k-vagante-fullcopy-20261017/filesystem/`.
Se compararon los 2,124 archivos restantes por SHA-256 contra el origen; no
hubo diferencias. El VPK 01.36 se instaló en otro clon completo de prueba,
junto con la entrega ACT_ONE.

Con el perfil aislado corregido, SDL y Qt no crearon ventana ni log de sesión;
`--help` tampoco produjo salida. La prueba VPK 01.36 sobre el clon completo
terminó con `0xC0000005` antes de crear una sesión nativa en 0.2.1.4190 y,
repitiendo la misma prueba con 0.2.1.4183 desde `_local/vita3k`, volvió a
terminar con el mismo código. Ambos clones completos tenían su propio
`pref-path`; usaron OpenGL/SDL, instalación por title ID y el replay completo
de 494 pasos con las tres trazas actualizadas tras la pantalla de victoria. Los
cinco logs de cada ejecución quedaron vacíos, así que no hay indicio de que
Vita3K llegara a cargar el VPK. La prueba final 4190 está en:
`_local/vita3k-act-one-136-run-final2/runs/iteration136/run-000001/`.

La derrota, la supervivencia del jugador y la salida posterior están validadas
en simulación host, no dentro de Vita3K. La instancia Vita3K de Touhou123 no se
cerró ni reutilizó.

## Pendiente

- Diagnosticar el `0xC0000005` de Vita3K y consumir el replay de 494 pasos con
  el paquete de campaña 01.36.
- Confirmar el spawn del jefe, el daño, la recompensa y la transición con una
  fuente nativa o evidencia de Ghidra adicional; el modelo host sigue siendo
  provisional.
- Verificar si el contenido de los tres pisos representa el primer mundo
  completo del juego original; los paquetes actuales aún no prueban paridad de
  generación.
