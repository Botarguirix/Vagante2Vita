# Iteración 081: rutas físicas ACT_ONE y correcciones de entidades

El cargador VWE2 ahora acepta `facingLeft` en DartTrap y BoulderBlocker, y el
exportador coloca `canReload` en el bit que consulta el runtime. Antes, el bit
de orientación hacía que el piso 2 fallara al cargar. La placa Boulder ahora
trata la presión inicial como estado ya presionado y conserva la placa abajo
después de activar la trampa. También se verifica el impacto de flecha cuando
su posición de lanzamiento ya intersecta un enemigo.

Con GCC 15.2 en WSL compilé y ejecuté el probe `player_step` sobre los tres
mapas actuales, semilla 12345. Los tres llegan a su marcador de salida y
producen trazas VTR1, proofs v3 y una replay con fuentes físicas en todos los
pisos:

| Piso | Ruta | Ticks | Saltos | Estado al tocar salida |
|---|---:|---:|---:|---|
| 0 | 56 tiles | 1,741 | 14 | vivo y apoyado |
| 1 | 64 tiles | 736 | 13 | vivo, aún en caída |
| 2 | 39 tiles | 375 | 0 | vivo, en escalera |

El paquete `_local/delivery-act-one-campaign082` valida su manifiesto de 100
archivos. Las pruebas están ligadas al SHA-256 de cada mapa, entidades,
simulador y traza; el fingerprint actual del simulador es
`12c3bc03d32506864f1c82996fe196e64437dabf065c801578d61570895b5ee0`. La replay
`_local/act-one-campaign-exit-082.vts` contiene 545 segmentos y 3,184 ticks,
con `route_sources=player_step:0,player_step:1,player_step:2`.

Compilé el VPK `00.81` con VitaSDK 2026.08 y GCC 15.2 tanto en WSL como en
Windows. `check_vpk.py` confirma en ambos `TITLE_ID=VAGV00006`, `APP_VER=00.81`,
siete entradas y cero errores o avisos. Ambos contenedores miden 830,158 bytes;
sus siete archivos internos son idénticos byte por byte. El SHA-256 del VPK
WSL es `046B694CDB517C22127C59F945CD670E1062253A2D66AB7187E226D4ACC98A62` y el
de Windows es `47815F83E10739EE8BBAD8E470DB6E2810F2938FB1A1626020F8F3A114D527FB`.
La compilación WSL usa una copia temporal del código bajo `/tmp/vagantevita-081`
porque el empaquetador rompe las rutas del proyecto cuando contienen espacios.
Las 12 pruebas C host y las 159 pruebas Python pasan.

El smoke automatizado valida el manifiesto y prepara la replay física, pero no
ha conseguido arrancar el VPK actual en Vita3K. La primera ejecución 4183 en
modo consola terminó por timeout antes de registrar una sesión. Las ejecuciones
posteriores quedaron activas sin ventana, CPU adicional o log: 4183 con Qt y
SDL, y 4170 con SDL. Se detuvieron al comprobar que no había progreso. El
frontend visible de Vita3K no es utilizable en el contexto automatizado actual;
los runs registran instalación intentada por ID, perfil aislado, hash del VPK,
configuración y ruta del replay. Los resultados están en
`_local/vita3k-vagante-clone/runs/iteration081/run-000001` a `run-000007`.
Estos intentos no validan el VPK en Vita3K.

La auditoría ahora separa los draws conocidos, las entidades que no consumen
RNG en su etapa de room, los libros cuyo RNG se consume después de recorrer los
rooms y los tipos aún desconocidos. Ghidra verificó que los tipos Doodad
1/2/4/5/19/21, BoulderTrap y DartTrap presentes en la semilla 12345 no hacen
draws en sus constructores/`setLevel`; Doodad 14 se clasifica como libro
diferido. Los pisos 0/1/2 tienen respectivamente 44/29/45 entidades, 34/20/36
draws conocidos, 9/8/8 entidades RNG-free, un libro diferido por piso y cero
tipos desconocidos. No cambia el VPK 00.81.

La inspección headless de Ghidra 12.1.4 sobre el PE de referencia recuperó un
caso adicional: `Doodad::setTypeVars` (`0x0059a6e0`) consume dos valores MT
uniformes para `DOODAD_TORCH=3`, uno para el cuadro inicial y otro para el
temporizador. El ledger ahora consume ambos en `Room::createRoomEntities`, en
el orden de las definiciones del room; la prueba sintética confirma los dos
draws y sus razones. `Entity::Entity` y `Unit::Unit`, y los constructores/
`setLevel` de DartTrap y BoulderTrap, no muestran llamadas RNG en el código
exportado. Los tipos Doodad restantes de ACT_ONE tampoco tienen draws en
`setTypeVars`; el libro vacío se contabiliza en la etapa posterior de skills.
El fixture no se incorpora al VPK; siguen pendientes hooks de entidades no
incluidos en estos mapas, el evento Halloween cuando aplique y el empate de
`std::sort`.

ACT_ONE sigue incompleto. La generación exacta por semilla aún no está
reconstruida, los enemigos 1–2 siguen siendo provisionales, las rutas de esos
pisos terminan en caída/escalera y los cambios de piso todavía no se han
observado en Vita3K. Las trazas demuestran que `player_step` puede alcanzar los
marcadores con los paquetes actuales; no prueban una campaña terminada en el
emulador.
