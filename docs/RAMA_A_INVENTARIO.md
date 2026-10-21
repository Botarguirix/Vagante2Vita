# Rama A — Inventario del PDB y cobertura de Codex

Fecha: 20 de octubre de 2026. Fuente: `vagante.pdb` local (64 974 848 bytes,
EXE SHA-256 `9af1d8cc…4624c`). Herramientas: `tools/pdb_inventory.py` y
`tools/codex_coverage.py`; las salidas quedan en `_local/pdb/` (no se versionan).

## Qué contiene el PDB

| Dato | Valor |
|---|---|
| Registros de tipos (TPI) | 410 311 |
| Clases/structs completos | 13 161 (incluye STL, boost, SFML) |
| Enums | 1 608 |
| Módulos (`.obj`) | 1 067 |
| Módulos del juego | 237, en `C:\code\projectrogue\msvc\vagante\ReleaseStatic\` |
| Funciones propias del juego | 4 678 (deduplicadas por nombre) |
| Código propio del juego | 3 770 467 bytes x86 |
| Clases del juego con código | 386 |

El nombre interno del proyecto es **projectrogue**. Cada `.obj` corresponde a un
`.cpp` original (`Player`, `Level`, `LevelGenerator`, `BossDragon`…). Por cada
clase el PDB da tamaño, bases, offsets y tipos de miembros, y lista de métodos
virtuales. Es suficiente para reescribir las cabeceras `.h` originales casi sin
adivinar.

Las bibliotecas enlazadas son SFML, boost (filesystem, locale), curl, zlib,
Steamworks, rapidjson e imgui.

### Alcance que importa para Vita

| Conjunto | Funciones | Bytes x86 |
|---|---|---|
| Todo el juego | 4 678 | 3 770 467 |
| Sin editores/imgui (`LevelEditor`, `WeaponEditor`, `PaletteEditor`, `ConfigGui`, `UGCBrowser`, imgui) | 4 136 | 3 391 825 |
| Además sin red/Steam (`Client`, `Server`, `Network`, `Snapshot`, `Message`, `Achievements`) | 3 889 | 3 182 659 |

Las unidades más grandes son `Player` (269 KB), `Client`, `Item`, `Level`,
`LevelGenerator`, `Projectile`, `SpellEntity`, `ResourceManager`, `PlayerMenu`
y `BossFinal`.

## Cuánto avanzó Codex

Se buscaron los nombres `Clase::método` del PDB en la versión actual de
GitHub (184 documentos y `vita/src`, `tools`, `tests`):

| Métrica | Resultado |
|---|---|
| Funciones nombradas en algún archivo | 138 / 4 678 (**2,9 %**; 10,9 % de bytes) |
| Funciones nombradas en código, herramientas o pruebas | 48 (**4,0 %** de bytes) |
| Unidades `.cpp` mencionadas | 55 / 225 |

"Nombrada" es una **cota superior**: casi siempre significa que Codex leyó la
función en Ghidra y copió algunas constantes a un motor C propio. No significa
que la función esté traducida. El roadmap de Codex marca IA, jefes y generación
como "provisionales".

Las mayores coberturas parciales están en `ItemGenerator`, `Room`, `Item`,
`BossDragon`, `BossGoblinKing`, `BossWorm` y `LevelGenerator`. No se tocaron
`SpellEntity`, `ResourceManager`, `Inventory`, `Shrine`, `ShopKeeper`,
`PetDog`, ni 10 de los 13 jefes, entre otras.

**Conclusión:** como descompilación, Codex lleva menos del 5 % del código
del juego. Se sigue con la rama A.

## Qué se reaprovecha de Codex

- `tools/vra.py`: lector/extractor del contenedor `data.vra`.
- Scripts de Ghidra en `tools/ghidra/` y `run_decompile.ps1` (pseudocódigo de 30 568 funciones).
- Recuperación del RNG: MT19937 y sorteos float32 bit a bit (`generation_random.py`).
- `floor_layout.py` y afines: referencia parcial de `LevelGenerator` útil para validar.
- Empaquetado VPK y LiveArea (`vita/CMakeLists.txt`, `sce_sys/`).

Se descarta como base: el motor C con render por software (`sceDisplaySetFrameBuf`),
que no tiene GPU ni audio.

## Plan de la rama A

1. **Cabeceras desde el PDB:** generar los `.h` de las 386 clases del juego (layout, bases, vtables).
2. **Capa `sf::` compatible** sobre SDL2 + vitaGL + audio, para que el código reconstruido no cambie.
3. **Traducción por unidad** desde el pseudocódigo de Ghidra, en orden de dependencia:
   `GameEngine`/`ResourceManager`/`Input` → `Entity`/`Unit`/`Level` → `LevelGenerator` → `Player` → enemigos, objetos, jefes.
4. **Build de PC primero** (Windows, SDL2), comparando contra el juego original con la misma semilla; luego Vita.
5. Red y Steam desactivados al principio; editores fuera.
