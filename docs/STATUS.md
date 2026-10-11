# Estado del 10 de octubre de 2026

## Completado

- Instalación local Windows localizada y analizada sin modificar sus archivos.
- Inspector PE/MSF propio ejecutado: x86 PE32, PDB coincidente, 24 487 símbolos públicos.
- Extractor de recursos propio ejecutado: 2 705 registros; conserva los dos nombres duplicados.
- Ghidra 12.1.4 y Temurin JDK 21.0.12.1 descargados; SHA-256 verificados.
- Análisis Ghidra terminado y proyecto guardado. Primera pasada: 30 570 funciones internas reconocidas, 30 566 exportadas y 4 fallos. Incluye bibliotecas/runtime, no sólo lógica del juego.
- Reintento con 120 segundos: recuperó Item::setNameAndDescription y Player::step. **Resultado final: 30 568 funciones con pseudocódigo y 2 fallos por timeout**, ItemGenerator::ItemGenerator y Thief::confirmHeist. El índice final es `functions.tsv`; `summary.txt` conserva cifras de la primera pasada.
- Inventario Ghidra exportado: 110 391 tipos y layouts de campos. Se reportaron 2 828 conflictos de tipos; no implica 2 828 funciones fallidas. Algunas categorías de símbolos/tipos PDB no son compatibles con el analizador.
- `main` fue exportada en `0x006a0570`; incluye advertencia de propagación de tipos no estabilizada y requiere revisión manual.
- Seis pruebas del extractor pasan, incluyendo límites, truncamiento y rutas peligrosas.
- Aplicación de diagnóstico Vita compilada con GCC 15.2.0 y empaquetada en VPK.
- Repositorio GitHub público creado: [Botarguirix/Vagante2Vita](https://github.com/Botarguirix/Vagante2Vita).

## Pendiente

- Analizar manualmente los dos fallos persistentes: ItemGenerator::ItemGenerator (`0x0063cde0`) y Thief::confirmHeist (`0x007cb110`).
- Reconstruir código portable y verificar comportamiento contra el juego.
- Captura del usuario recibida: diagnóstico visible y encabezado zlib encontrado. Falta comprobar contenido completo, mando y rendimiento del juego; el overlay de la pantalla estática no es un benchmark de Vagante.
- Implementar un segmento jugable y medir rendimiento/memoria en Vita.

No existe una decompilación recompilable del juego ni un port jugable en este estado. Los archivos locales en `_local` permanecen excluidos de Git.

## Archivos locales entregados

- `_local/Vagante-decompilation.zip`: pseudocódigo, inventarios y estado final. Las comprobaciones del paquete verifican archivos y ZIP, no comportamiento.
- `_local/decompiled/`: salida sin comprimir, con `functions.tsv`, `types.tsv`, `fields.tsv`, `retry.tsv` y `verified_summary.json`.
- `_local/ghidra/Vagante.gpr`: proyecto guardado; mantener también su directorio `Vagante.rep`.
- `_local/assets/`: recursos de la copia local, con duplicados preservados.
- `build-vita/Vagante2Vita-diagnostic.vpk`: diagnóstico ARM, no juego. VPK generado y comprobado como ZIP; consola física pendiente.

## Iteración 2

Título nativo 00.02 compilado en Release. Verificador C de escritorio confirmó los 2.705 registros, 124.879.064 bytes descomprimidos y diez texturas. Seis pruebas Python pasan. El VPK se comprobó como ZIP íntegro: 95.111 bytes, SHA-256 6e66f63f544133649fd3c5a4f4d462019ad28521c7044f1afc58841f8652a28b.

Ver docs/ITERATION2.md y docs/ROADMAP.md. La pantalla y controles de 00.02 siguen pendientes de prueba física; el juego no es jugable. El archivo nuevo es build-vita/Vagante2Vita-title.vpk.
