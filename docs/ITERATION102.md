# Iteración 102: emboscada del Lurker y VPK 01.02

El Lurker dejó de usar la persecución genérica. Ghidra muestra que el
constructor inicia `attackDelayTimer` en 16 ticks y establece daño base de
4–16; `Lurker::step` espera que se dispare el sensor y termine el retraso antes
de reproducir el mordisco, y `Lurker::doCollision` entrega un evento de daño
único durante el ataque. El runtime ahora prepara 16 ticks, abre una ventana de
mordida, aplica el rango 4–16 una sola vez y permanece en el sitio. La distancia
que activa el sensor, la ventana visual y la recuperación se aproximan (la
recuperación local usa 32 ticks), pues el sensor/retract sprite no están
representados en el paquete de salas.

CTest pasó 12/12 en WSL, incluidos los tests de aviso, daño, golpe único,
recuperación y daño integrado al jugador. La suite Python pasó 170/170 con el
Python 3.12.14 del runtime local. El VPK 01.02 compila con VitaSDK. Los proofs
VTR1 de los tres pisos se regeneraron con el código actual y el paquete privado
valida 113 archivos; la replay de campaña valida 545 segmentos y 3,184 ticks.

El smoke aislado de Vita3K 0.2.1 SDL/OpenGL con 01.02 agotó 30 segundos antes
de crear sesión nativa; stdout, stderr y log del emulador quedaron vacíos. El
resultado está en `_local/vita3k-vagante-test102/runs/iteration102/run-000001/`.
Un build ARM y un replay preparado no prueban que el runtime haya arrancado.
ACT_ONE sigue incompleto: faltan paridad de IA y generación, la transición de
campaña verificada en runtime y pruebas en Vita real.
