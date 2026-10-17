# Iteración 074: colisión de Bat basada en Ghidra

La colisión de Bat ya no comparte el daño genérico provisional que antes
afectaba a todos los enemigos. `Bat::Bat` establece `minDamage=1` y
`maxDamage=6`; `Bat::doCollision` selecciona dentro de ese intervalo y fija
`lastHurtUnitFrames` en 30 tras un golpe aceptado. El runtime aplica ese rango
y conserva la ventana de 30 frames por enemigo. Goblin ya no daña al jugador
por simple contacto; su ataque con arma y los ataques de Lurker/Slime siguen
pendientes de reconstrucción individual.

El port usa un estado pseudoaleatorio determinista local a cada Bat, inicializado
desde su índice y posición. La fuente no es el MT global del juego, así que la
distribución se mantiene en 1–6 y es repetible, pero las tiradas no tienen
paridad exacta con la semilla original. El daño total pasa por el modificador
defensivo del jugador.

Se conserva el esquema documentado por [Repentogxm](https://github.com/0xl0cal/repentogxm/)
y [D2Vita](https://github.com/Franckrst/D2Vita/blob/main/ARCHITECTURE.md):
binario y datos comerciales identificados y mantenidos localmente, herramientas
de extracción/preparación en el host, y runtime Vita con paquetes de datos
externos verificados. Estas referencias describen arquitecturas específicas
para Isaac y Diablo II; no se reutiliza su traductor, runtime x86 ni extractor
MPQ. La revisión del encaje está en [PORTING_REFERENCE_REVIEW.md](PORTING_REFERENCE_REVIEW.md).

El bundle privado de ACT_ONE 073 se reutilizó sin cambios y validó 94 archivos.
El VPK 00.74 se compiló con VitaSDK en
`_local/build-act-one-074/Vagante2Vita-iteration074.vpk` (827,411 bytes), SHA-256
`EA803725F0AE8B6E77665320E05860D5DDF667FC6959BF45BEF78A28E1D2E0D5`. El
verificador de VPK aceptó sus siete entradas, TITLE_ID `VAGV00006`, versión
`00.74` y LiveArea.

La suite Python pasa 140/140. `world_combat.c`, `world_entities_test.c` y
`player_test.c` compilan para ARM con `-Wall -Wextra -Werror`.

La prueba aislada preparó la replay de salida ACT_ONE (34 pasos, ruta de 56
tiles), pero Vita3K agotó 45 segundos y salió con código `-1` antes de crear la
sesión nativa. `stdout.log`, `stderr.log`, `vita3k.log` y los logs de depuración
están vacíos. Esto no valida el arranque ni el gameplay del VPK. El primer
mundo sigue incompleto: faltan ataques de otros enemigos, más reglas de
generación y una replay reproducible dentro del runtime.
