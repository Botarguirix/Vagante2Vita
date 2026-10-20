# Iteración 132: Decompedia y aislamiento del análisis Ghidra

La revisión de [Decompedia](https://decomp.wiki/) confirma que el sitio es un
índice de proyectos, plataformas y recursos comunitarios, no un decompilador ni
un repositorio de Vagante. Su material sobre *matching decompilation* apunta a
reconstruir código que produzca el mismo binario con el toolchain original;
VaganteVita hace reconstrucción funcional para ARM/Vita, así que la igualdad de
bytes no es una meta adecuada. La utilidad concreta para nosotros es mantener
procedencia por función, contrastar flujo de control y tipos con el binario, y
evaluar por separado la evidencia estática, las pruebas host y el runtime Vita.
Fuentes metodológicas: [Decompedia](https://decomp.wiki/) y la [visión general
del análisis de decompilación](https://decompilation.wiki/fundamentals/overview/).

Se comprobó el entorno Ghidra local para continuar la auditoría de salud del
Goblin King. El proyecto existente declara como propietario `ikari`, mientras
que el proceso actual se ejecuta como `AndyPC`; Ghidra rechaza abrir esa base.
Una copia de reimportación conserva metadatos pero no la base de programa. El
primer intento de importación aislada desde `_local/input/vagante.exe` y su
PDB fue interrumpido antes de guardar. La revisión continuó con una copia
privada de la base Ghidra existente, que conserva el análisis completo; al
ajustar OWNER sólo dentro de `_local/analysis132/ghidra-copy`, Ghidra pudo
exportar los listings de instrucciones sin tocar la base original. La
recuperación de stats y su implementación se registran en
[ITERATION133.md](ITERATION133.md).

No se subieron EXE, PDB ni pseudocódigo a servicios compartidos. La falla de
importación limpia queda como nota de operación; no impidió continuar desde la
copia local de Ghidra.
