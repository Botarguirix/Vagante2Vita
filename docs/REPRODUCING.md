# Reproducir el análisis

Usar una copia propia de Vagante. Python 3.10+ para inspectores; Windows PowerShell para el lanzador Ghidra.

1. Ejecutar `tools/inspect_game.py GAME_DIRECTORY`. Produce `_local/analysis/binary.json` y `symbols.json`. El PDB debe coincidir con GUID y age del EXE antes de usar direcciones y tipos.
2. Ejecutar `tools/vra.py GAME_DIRECTORY/data.vra`. Produce inventario con hashes y tamaños. `--extract _local/assets` extrae recursos a un directorio nuevo y preserva duplicados en `_duplicates/ORDINAL/`. No repaqueta ni modifica Steam. Un directorio con archivos existentes causa error; usar otro directorio para repetir.
3. Ejecutar `tools/setup_re.py`. Descarga Ghidra 12.1.4 y JDK 21.0.12.1 oficiales en `_local/toolchain`; requiere red y espacio libre. Verifica SHA-256 de Java y el digest de Ghidra cuando GitHub lo ofrece. Las herramientas descargadas no se publican. Python 3.11+ es necesario para este instalador.
4. Ejecutar `./tools/run_decompile.ps1 -GameDirectory 'RUTA'`. Copia EXE/PDB a `_local/input` para evitar problemas del lanzador batch con paréntesis en la ruta de Steam. Importa y analiza, luego exporta con `tools/ghidra/ExportDecomp.java`. Puede escribir configuración de Ghidra en el perfil de usuario. El proyecto local Vagante se sobrescribe al repetir.
5. Revisar `_local/ghidra/analysis.log`, `_local/decompiled/functions.tsv` y `summary.txt`. Si el análisis agota 1 800 segundos o hay advertencias de tipos/flow, las funciones correspondientes quedan sin validar. El límite por función de decompilación es 30 segundos. `-MaxFunctions N` limita la exportación; 0 intenta todas las funciones internas reconocidas.

Cada `.c` exportado está identificado por dirección y contiene pseudocódigo. Los nombres/tipos PDB ayudan, pero offsets, excepciones, RTTI y llamadas MSVC requieren revisión. No anunciar resultados como código original ni fuente ARM lista para compilar.

## Pruebas

`python -m unittest discover -s tests -v` verifica truncamiento, rutas peligrosas, límites y duplicados del extractor con archivos sintéticos. La ejecución real del inspector y extractor contra el juego verifica compatibilidad de esta copia; no se sube al CI.

## GitHub

El repositorio solicitado es `Botarguirix/Vagante2Vita`. `tools/create_github_repo.py` permite crearlo utilizando Git Credential Manager y GitHub REST; no imprime ni guarda credenciales. Publicación de fuentes propias mediante Git. Antes de publicar, revisar archivos rastreados y verificar que `_local` permanece ignorado.
