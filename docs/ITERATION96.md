# Iteración 096: regla de colisión de salto para Spider

Ghidra recupera en `Spider::doCollision` el contrato de daño de contacto:
Spider solo golpea durante `ACT_JUMP`, con rapidez absoluta de al menos 0.2;
el daño se sortea entre 5 y 10, y un mapa por objetivo evita
golpes repetidos durante 30 ticks. Si el método `hurt()` devuelve 1, el código
nativo aplica `STATUS_POISON` durante 300 ticks. La caja del Spider es 10×8 y
el constructor fija esos límites de daño. El proyecto añade la regla de daño
y su período de inmunidad por Spider.

El cargador VWE3 y el runtime todavía no recuperan la IA que entra en
`ACT_JUMP`, ni el estado de movimiento necesario para activar esta colisión.
Por ello, la nueva regla es una pieza del contrato de combate y todavía no
hace que Spider sea una amenaza jugable. `Spider::step` delega la física a
`Monster::step`; el lanzamiento del salto, su animación y los filtros de
colisión aún requieren una reconstrucción coordinada. El disparo de telaraña
también sigue pendiente: Ghidra identifica `PROJECTILE_WEB_BALL`, el sprite y
el constructor, pero los efectos de impacto no están suficientemente
resueltos para implementarlos con fidelidad.

El runtime y la prueba C compilan para ARM con VitaSDK y
`-Wall -Wextra -Werror`. Las 170 pruebas Python pasan. La prueba C aún no pudo
ejecutarse en host porque esta sesión no tiene compilador host ni CMake; la
comprobación de WSL devuelve `E_ACCESSDENIED` al enumerar distribuciones. Se
generó el VPK 00.96: 830,546 bytes, SHA-256
`CB472B008009A6A498A64B57B67AF406007186C58DCFBF8F0ED1C6B302B096F4`.

La validación automatizada en Vita3K sigue pendiente. Los intentos de 0.2.1.4170
y 0.2.1.4183 descritos en [ITERATION95.md](ITERATION95.md) terminan o expiran
antes de crear una sesión nativa y dejan logs vacíos. No se atribuye ese fallo
al juego. ACT_ONE no está completo: faltan la IA y ataques de Spider, otras
reglas de combate/enemigos, paridad de generación y una campaña reproducida en
Vita3K o hardware.
