# Iteración 031: daño melee limitado por animación de ataque

El paso de combate de ACT_ONE aplica como máximo un impacto por enemigo para
cada `MeleeState.serial` y respeta un cooldown provisional de 12 ticks entre
impactos. El serial evita que una animación activa vuelva a golpear al vencer
el cooldown; el cooldown bloquea ataques distintos que coincidan durante la
ventana temporal.

`world_entities_test.c` verifica que el primer ataque reduzca HP de 3 a 2, un
ataque nuevo inmediato quede bloqueado, el siguiente ataque impacte tras el
cooldown y el mismo serial no repita daño después de vencerlo. Un tercer ataque
puede derrotar al enemigo. El test compila como ELF ARM con VitaSDK y
`-Wall -Wextra -Werror`; no hay compilador C host instalado para ejecutarlo.
La suite Python pasó 107/107. Se generó el VPK 00.31 localmente, sin recursos
comerciales.

Se intentó una replay de salida ACT_ONE en la copia privada Vita3K. El emulador
superó 45 segundos sin ventana, sin log de emulador ni sesión nativa; su proceso
permaneció ocioso. El harness ahora acepta `-TimeoutSeconds`, cierra solamente
el proceso del clon cuando vence y guarda stdout/stderr y resultado terminal.
Una prueba separada con timeout de 5 segundos verificó esa limpieza; no valida
el gameplay. La replay completa no alcanzó aserciones y no valida el VPK en
ejecución.

SHA-256 del VPK:
`389DB39054B8CD235D094910889CD4F360C463B4CBDD6CA337D0B3DC11EFDEC9`.

Esto mejora el comportamiento provisional del enemigo, pero no constituye
combate fiel a Vagante. Siguen pendientes los frames de invulnerabilidad,
knockback por polígono, animaciones/estados originales, loot, transición de piso
y ejecución de las pruebas nativas en host o Vita3K. WSL no inicia en este
entorno (`E_ACCESSDENIED`), así que la ruta habitual para agregar un compilador
host también queda pendiente.
