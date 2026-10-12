# Vagante2Vita

Investigación técnica e ingeniería inversa reproducible para un posible port nativo de **Vagante a PS Vita**.

**Estado:** reconstrucción parcial; la iteración 013 permite una primera prueba de recorrido del mundo inicial con movimiento y atributos base. No es todavía un port completo ni una decompilación recompilable. El ejecutable local es Windows x86 de 32 bits, con C++/SFML, OpenGL, OpenAL y Steamworks. Se encontró un PDB coincidente que permite recuperar nombres y tipos. La reconstrucción de la lógica y la adaptación a ARM siguen pendientes.

Se generó localmente pseudocódigo de **30 568 funciones**, incluidos código del juego y bibliotecas/runtime, con dos fallos por timeout después de reintentar. El título 00.02 fue probado en Vita por el usuario. La iteración 00.03 añade fondo animado, sprites por atlas, vista parcial de selección de clases y log de recursos, transiciones y rendimiento; falta probar esta versión en consola. Estos resultados no representan un porcentaje de reconstrucción ni prueban rendimiento del juego en Vita.

- [Viabilidad técnica y evidencia](docs/FEASIBILITY.md)
- [Proyectos relacionados](docs/REFERENCES.md)
- [Plan de reconstrucción](docs/ROADMAP.md)
- [Entrega 013: primer recorrido interactivo del mundo inicial](docs/ITERATION13.md)
- [Entrega 008: paisaje en caché y diagnóstico de stuttering](docs/ITERATION8.md)
- [Primera prueba jugable: zona inicial, colisiones y jugador](docs/FIRST_GAMEPLAY.md)
- [Entrega 007: actualizar la misma aplicación](docs/ITERATION7.md)
- [Arte de LiveArea](docs/ITERATION6.md)
- [Optimización 005: resolución reducida y caché](docs/ITERATION5.md)
- [Conservación de iteraciones y logs](docs/ITERATION4.md)
- [Cambios del menú animado 003](docs/ITERATION3.md)
- [Cómo reproducir el análisis](docs/REPRODUCING.md)
- [Resultados de esta ejecución](docs/STATUS.md)
- [Compilar y usar el diagnóstico Vita](docs/VITA_DIAGNOSTIC.md)

## Herramientas

Python 3.11 o posterior; las herramientas Python no requieren paquetes externos.

```powershell
python tools/inspect_game.py 'C:\Program Files (x86)\Steam\steamapps\common\vagante'
python tools/vra.py 'C:\Program Files (x86)\Steam\steamapps\common\vagante\data.vra'
python tools/vra.py 'C:\Program Files (x86)\Steam\steamapps\common\vagante\data.vra' --extract _local/assets
python tools/setup_re.py
./tools/run_decompile.ps1
python -m unittest discover -s tests -v
```

Las salidas se guardan en `_local/`: inventario PE/PDB, símbolos, recursos extraídos, proyecto Ghidra y pseudocódigo C. Los resultados de Ghidra deben revisarse función por función; no equivalen al código C++ original ni se pueden compilar automáticamente para Vita.

## Alcance del repositorio

Se publican herramientas originales, documentación y el diagnóstico Vita. Cada usuario aporta su copia del juego. No se incluyen ejecutables, PDB, gráficos, audio, guardados ni pseudocódigo comercial. La licencia de las herramientas no concede derechos sobre Vagante. Proyecto no afiliado con Nuke Nine.

La entrega actual 013 conserva la misma aplicación y separa entregas/logs: [instrucciones](docs/ITERATION13.md).


Última entrega: **iteración 014**, misma aplicación VAGV00006. Optimiza escalado, transferencia de pantalla y terreno; objetivo 60 Hz pendiente de validación física. [Instalación y resultados](docs/ITERATION14.md).


Última entrega local: **015**, con mundo inicial incluido en el VPK privado y fallback `app0:world`. [Instalación, controles y alcance](docs/ITERATION15.md). Los recursos comerciales no forman parte del repositorio; builds públicos requieren preparar la copia del usuario.


**Entrega vigente: 016.** VPK sin recursos comerciales, mundo externo, pausa START y corrección de aterrizajes. Sin métricas integradas. [Detalles](docs/ITERATION16.md).


**Entrega vigente: 017.** Primera adaptación de bordes, recursos hang externos opcionales. [Instalación y alcance](docs/ITERATION17.md). Objetivo ampliado al mundo completo; generación, entidades y combate siguen pendientes.


**Entrega vigente: 018.** Fondo de menú animado por defecto, R alterna perfil ligero para medición externa. [Detalles](docs/ITERATION18.md).


**Entrega vigente: 019.** Selección tras comparaciones locales: ligero por defecto, animado preparado disponible por R. Intro jugable; todavía no juego completo. [Validación e instalación](docs/ITERATION19.md).
