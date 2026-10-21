# Comparación: rama A (reconstrucción desde PDB) vs rama B (motor de Codex)

Fecha: 20 de octubre de 2026. Todas las cifras se midieron en esta máquina sobre
`vagante.pdb`, el repo de Codex (`main`, commit `d0c4768`) y su pseudocódigo en
`VaganteVita\_local\decompiled`.

## Resumen

| | Rama B — Codex | Rama A — PDB |
|---|---|---|
| Enfoque | Motor nuevo en C que **imita** el juego | **Reconstruir** el C++ original clase por clase |
| Hoy se puede jugar | **Sí**: intro + ACT_ONE (3 pisos) | **No** |
| Fidelidad | Aproximada; IA, jefes y generación son "provisionales" | Layout exacto verificado (6 487 comprobaciones) |
| Cobertura del juego | < 5 % del código (138 de 4 678 funciones nombradas) | 100 % de los tipos; 0 % de las funciones traducidas |
| Render | Software, CPU (`sceDisplaySetFrameBuf`) | GPU (vitaGL) — por hacer |
| Audio | No hay | SDL2/OpenAL/vorbis — por hacer |
| Camino al juego completo | Rehacer a mano cada sistema | Traducir 4 136 funciones con su forma original |

**Recomendación: seguir con la rama A como base y reaprovechar las piezas
buenas de Codex.** La rama B sirve como demo jugable y como referencia, pero no
escala al juego completo.

## Rama B — motor de Codex

### A favor
- **Ya existe algo jugable** y se probó en Vita real (título 00.02): intro, cueva,
  ACT_ONE con tres pisos, enemigos, tres jefes, melee, arco, cofres y loot.
- **Mucha infraestructura de pruebas**: 14 suites C host, 226 pruebas Python,
  replays deterministas y rutas físicas automatizadas.
- **Hallazgos valiosos del juego**: RNG MT19937 con sorteos float32 exactos bit
  a bit, partes de `LevelGenerator` (salas, agua, secretos, endpoints) y lector de `data.vra`.
- VPK pequeño (~830 KB) y empaquetado con LiveArea resuelto.

### En contra
- **No es el juego, es una imitación.** Cada enemigo y jefe se reescribe con
  constantes sueltas copiadas de Ghidra. El propio roadmap marca IA, jefes y
  generación como provisionales.
- **No escala.** Más de 200 iteraciones para llegar a parte del primer acto.
  Quedan sin tocar 10 de 13 jefes, `SpellEntity`, `Inventory`, `Shrine`,
  `ShopKeeper`, mascotas, tiendas, guardado, menús y multijugador.
- **Render por CPU**: ya bajó la resolución a 640×360 / 320×180 para rendir.
  Con luces, partículas y el shader del juego no alcanzará.
- **Sin audio.**
- **Arquitectura distinta** a la original (C plano, `world_combat.c` de 900+ líneas
  con varios enemigos mezclados). Cuanto más crezca, más difícil será acercarla al original.
- **Buena parte del esfuerzo reciente se fue en Vita3K**, que en este equipo
  falla incluso sin cargar ningún juego.

## Rama A — reconstrucción desde el PDB

### A favor
- **Exactitud verificable desde el día uno.** 571 tipos con nombres, offsets,
  herencia, enums y vtables originales; Clang confirma que el layout coincide con
  el ejecutable.
- **Lista completa de trabajo.** Se sabe exactamente qué falta: 4 136 funciones
  en 225 unidades `.cpp`, cada una con nombre, tamaño y pseudocódigo.
- **El pseudocódigo ya existe**: hay pseudocódigo para 4 104 de las 4 678
  funciones (lo generó Codex). Con las cabeceras, Ghidra puede reanalizarlo con
  tipos correctos, lo que lo hace mucho más legible.
- **Cada función traducida es la original**, no una aproximación: el
  comportamiento (IA, daño, generación, loot) sale igual sin tener que redescubrirlo.
- **GPU y audio desde el diseño** (SDL2 + vitaGL + OpenAL/vorbis, ya instalados).
- **Validación en PC**: el mismo código compila para Windows y se compara contra
  el juego original con la misma semilla, sin depender de Vita3K.
- La arquitectura original facilita añadir contenido después (todos los actos,
  objetos y jefes salen de las mismas clases).

### En contra
- **No hay nada jugable todavía**, y tardará en haberlo. Para ver algo en pantalla
  hace falta un núcleo mínimo: `GameEngine`, `ResourceManager`, `Input`, `Level`,
  `Entity`, `Unit`, `Player` y la capa `sf::`.
- **Volumen grande.** El pseudocódigo del juego (sin editores ni imgui) suma
  **~704 000 líneas**. Está inflado por STL/boost en línea; el código fuente real
  probablemente es varias veces más pequeño (estimación, no medición).
- **Hay que reimplementar las librerías**: un subconjunto de SFML (sprites,
  texturas, render targets, sonido, fuentes) sobre SDL2/vitaGL, partes de
  boost (filesystem, locale) y desactivar Steam y red. rapidjson es solo
  cabeceras y se puede usar tal cual.
- **Riesgos propios de MSVC**: determinismo de floats, orden de evaluación, tamaños
  de `wchar_t`/`long double` y contenedores STL distintos en ARM. Codex ya
  resolvió parte del tema del RNG/float32.
- Memoria en Vita sin medir: `Player` ocupa 31 KB por instancia (aceptable),
  pero el uso total de texturas está por verificar.

## Qué se reaprovecha de Codex en la rama A

| Pieza | Uso en la rama A |
|---|---|
| `tools/vra.py`, extracción de assets | Directo: carga de recursos |
| Pseudocódigo de 30 568 funciones y proyecto Ghidra | Fuente de la traducción |
| RNG MT19937 + float32 exacto | Referencia para validar `LevelGenerator` |
| Análisis Python de generación (`floor_*.py`) | Oráculo de pruebas: comparar pisos generados |
| Pruebas de física/rutas y replays | Método de validación |
| Empaquetado VPK, LiveArea, arte | Directo |
| La demo 02.03 | Demo jugable mientras la rama A madura |

## Riesgos de cada decisión

- **Seguir solo con B:** riesgo alto de quedarse en una demo del primer acto que
  "se parece" a Vagante, sin camino claro al juego completo ni al rendimiento.
- **Seguir solo con A:** riesgo de pasar mucho tiempo sin nada visible. Se mitiga
  con hitos cortos y verificables (abajo).

## Hitos propuestos para la rama A (puntos de control)

Si un hito se atasca, se reevalúa la estrategia.

1. **Capa de compatibilidad**: cabeceras compilando en modo port (PC) con la STL real
   y `sf::` propio.
2. **Primer arranque en PC**: `GameEngine` + `ResourceManager` + `Input` cargan
   `data.vra` y muestran el menú principal.
3. **Primer nivel en PC**: `Level` + `LevelGenerator` + `Entity`/`Unit`/`Player`;
   comparar la generación contra el original y contra los oráculos de Codex.
4. **Primer VPK de la rama A** en Vita física: menú + nivel, midiendo FPS y RAM.
5. A partir de ahí, traducción por unidades: enemigos, objetos, jefes, menús, guardado y audio.

## Cómo pueden trabajar juntos Codex y Claude

- Codex sigue en `VaganteVita` (`main`) con la demo, sin tocar la rama A.
- En la rama A, Codex puede encargarse de tareas acotadas: traducir una unidad
  concreta desde el pseudocódigo con las cabeceras como contrato, escribir sus
  pruebas y reanalizar en Ghidra con los tipos nuevos.
- Todo se valida igual: compila, pasa el layout check y se compara contra el original.
