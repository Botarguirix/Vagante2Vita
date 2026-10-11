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

## Iteración 3

El usuario confirmó arranque y título en Vita para 00.02. La versión 00.03 incorpora 22 texturas, animación por atlas/parallax, dos framebuffers, selección parcial de las tres clases base y log periódico. New Game ya abre esa vista; confirmar una clase registra explícitamente la falta de reconstrucción del arranque del mundo.

Probe C: 2.705 registros íntegros, 22 PNG (5.128.264 bytes RGBA), fotogramas distintos y composición de clases. Test C de transiciones pasa; seis pruebas Python y cuatro rechazos de archivo inválido pasan. Compilación Vita Release completada. Falta prueba física de 00.03 y medición real.

VPK `build-vita/Vagante2Vita-menu.vpk`, SHA-256 `dec545ea0282c9bc02ed92fe0df01e1758fb66335e740ab0b9acfc71300b6cf0`. Instrucciones y lectura del log: [ITERATION3.md](ITERATION3.md).

## Iteración 004: entregas y logs independientes

VPKs 001–003 preservados en artifacts/iteration001, iteration002 e iteration003. Nueva entrega 004 con título VAGV00004, instalable junto a la aplicación anterior. Logs creados con O_EXCL en logs/iteration004, numerados por sesión y sin borrado de anteriores. Test de preservación y navegación pasan; compilación Vita y ZIP/SFO verificados. SHA-256 VPK: d76dcc2da9004b44681f9fad6146da82b5c2d6f4404ef7c52919cc36e3b9014b. Pendiente prueba física 004.

Empaquetador local preserva VPK, snapshot del commit, roadmap, build.log y hashes; rechaza sobrescribir una carpeta de iteración. Ver [ITERATION4.md](ITERATION4.md). No hay cambios de gameplay en esta entrega.

## Iteración 005: render y carga

El log recibido pertenece a 00.02 y muestra 47,636 s de carga; no contiene PERF del menú animado. El usuario confirma animación y CPU alto, pero no se infiere un porcentaje CPU del archivo. 005 compone por defecto a 320×180 y alterna con 640×360 por L; añade tiempos separados de composición/escalado y espera de mando a vblank.

Caché privada de 22 PNG: 153.245 bytes comprimidos; mantiene 5.128.264 bytes RGBA. Probe confirma archivo completo y caché; regresión sintética e integridad corrupta/truncada pasan. Benchmark de escritorio: 4.123 frente a 1.026 µs por composición (no Vita). Navegación, logs y seis tests Python pasan. VPK Release ZIP/SFO verificados; falta prueba física 005. SHA-256: c9c4a430a874e0b05c2d8bc90b84ef6cd076caa6c63c91f107c805a83c31db51.

Ver [ITERATION5.md](ITERATION5.md). La caché se entrega sólo localmente junto al VPK, excluida de Git.
