# Iteración 101: proyectil web del Spider y VPK 01.01

El Spider ahora puede seleccionar un disparo al azar dentro del rango cercano,
prepararlo durante 24 ticks (aproximación mientras no esté disponible la
animación original), y lanzar un WEB_BALL dirigido. El análisis local de Ghidra
identifica ProjectileType 19, daño físico 7 y el SpellEntity de telaraña tipo 5
al impactar un objetivo. El simulador aplica 7 de daño, inmoviliza al jugador
por 90 ticks, barre cuatro subpasos por frame para evitar atravesar cuerpos y
adhiere el proyectil a una superficie sólida. La selección del 10% y los 24
ticks de preparación siguen siendo aproximaciones de `willWeb` y `sprWeb.done`.
Los tests C verifican el daño/impacto y la colisión con terreno; el test de
jugador confirma que el impacto resta 7 HP y bloquea movimiento en ese tick.

CTest pasó 12/12 en WSL. La suite Python ejecutó 156 tests, pero dos módulos no
pudieron importarse porque el Python instalado en WSL carece de Pillow, pip y
venv. El workflow de GitHub Actions ahora instala Pillow explícitamente antes
de ejecutar la suite. El VPK 01.01 se compila con VitaSDK y contiene solamente
los siete archivos permitidos: `eboot.bin`, `param.sfo` y los recursos LiveArea.

Se regeneraron pruebas y trazas VTR1 para los tres pisos con el simulador actual;
las rutas de 1,741, 736 y 375 ticks alcanzaron sus salidas. La entrega privada
final valida 113 archivos y el replay se consume en 545 segmentos/3,184 ticks.
Vita3K 0.2.1, SDL/OpenGL, volvió a terminar con `0xC0000005` antes de crear una
sesión nativa. El resultado está en
`_local/vita3k-vagante-test101-final/runs/iteration101/run-000001/`.
El nuevo VPK tiene build VitaSDK, pruebas host y replay preparadas, pero runtime
en Vita3K y hardware sin validar. ACT_ONE sigue incompleto; IA, tiempos de
animación, estado completo de `SpellEntity` y las transiciones requieren paridad
adicional con el juego original.

Se revisó [Decompedia](https://decomp.wiki/): su énfasis en decompilación
matching y en documentar compiladores/plataformas motiva nuestra trazabilidad
por hash, dirección, listing y nivel de evidencia. Sus ejemplos de matching y
herramientas para consolas no aplican directamente al ejecutable Win32 de
Vagante ni al objetivo de reconstrucción funcional para Vita. No se subieron
datos privados a servicios externos. Análisis: [REFERENCE_REPOSITORIES.md](REFERENCE_REPOSITORIES.md).
