# Estado del 10 de octubre de 2026

## Completado

- Instalación local Windows localizada y analizada sin modificar sus archivos.
- Inspector PE/MSF propio ejecutado: x86 PE32, PDB coincidente, 24 487 símbolos públicos.
- Extractor de recursos propio ejecutado: 2 705 registros; conserva los dos nombres duplicados.
- Ghidra 12.1.4 y Temurin JDK 21.0.12.1 descargados; SHA-256 verificados.
- Proyecto Ghidra iniciado; PDB cargado y tipos/símbolos principales procesados. Se reportaron 2 828 conflictos de tipos; no implica 2 828 funciones fallidas.
- Seis pruebas del extractor pasan, incluyendo límites, truncamiento y rutas peligrosas.
- Aplicación de diagnóstico Vita compilada con GCC 15.2.0 y empaquetada en VPK.
- Repositorio GitHub público creado: [Botarguirix/Vagante2Vita](https://github.com/Botarguirix/Vagante2Vita).

## Pendiente

- Finalizar el análisis automático y contabilizar exportación de pseudocódigo.
- Reconstruir código portable y verificar comportamiento contra el juego.
- Probar el diagnóstico en hardware físico; aún no se verificaron pantalla/mando en consola.
- Implementar un segmento jugable y medir rendimiento/memoria en Vita.

No existe una decompilación recompilable del juego ni un port jugable en este estado. Los archivos locales en `_local` permanecen excluidos de Git.
