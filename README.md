# Vagante2Vita

Investigación técnica e ingeniería inversa reproducible para un posible port nativo de **Vagante a PS Vita**.

**Estado:** no es un port jugable ni una decompilación recompilable. El ejecutable local es Windows x86 de 32 bits, con C++/SFML, OpenGL, OpenAL y Steamworks. Se encontró un PDB coincidente que permite recuperar nombres y tipos. La reconstrucción de la lógica y la adaptación a ARM siguen pendientes.

- [Viabilidad técnica y evidencia](docs/FEASIBILITY.md)
- [Proyectos relacionados](docs/REFERENCES.md)
- [Plan de reconstrucción](docs/ROADMAP.md)
- [Cómo reproducir el análisis](docs/REPRODUCING.md)
- [Resultados de esta ejecución](docs/STATUS.md)
- [Compilar y usar el diagnóstico Vita](docs/VITA_DIAGNOSTIC.md)

## Herramientas

Python 3.10 o posterior; los inspectores no requieren paquetes externos.

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
