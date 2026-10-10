# Proyectos relacionados

Búsqueda en GitHub y web el 10 de octubre de 2026. No se encontró un port/decompilación completa de Vagante en los resultados examinados. La búsqueda no demuestra ausencia universal; debe repetirse antes de adoptar una implementación.

| Proyecto | Utilidad y límites |
|---|---|
| [mtolly/vagante-extract](https://github.com/mtolly/vagante-extract) | Documenta y extrae/reempaqueta `data.vra`; no reconstruye lógica. El esquema zlib + registros se verificó contra la copia local. Nuestro extractor es una implementación independiente. |
| [raff-run/VaganteModLoader](https://github.com/raff-run/VaganteModLoader) | Modifica recursos apoyándose en vagante-extract; confirma una comunidad de mods. No es un motor ni una decompilación. |
| [ValiantWafer/VUltra](https://github.com/ValiantWafer/VUltra) | Mod con proxy OpenAL y parches de funciones Windows; documenta scripts que leen `vagante.pdb`. Referencia para localizar subsistemas; no es portable a Vita directamente. |
| [ekunuke/vagante_issues](https://github.com/ekunuke/vagante_issues) | Seguimiento archivado de problemas; no contiene fuente del juego. |
| [rsn8887/SDLPoP-Vita](https://github.com/rsn8887/SDLPoP-Vita) | Prince of Persia basado en desensamblado, trasladado a Vita. Precedente de reconstrucción nativa, sin reutilización directa de la lógica de Vagante. |
| [TheOfficialFloW/gtasa_vita](https://github.com/TheOfficialFloW/gtasa_vita) | Carga de código Android ARM con adaptación de APIs; útil para entender por qué se necesita arquitectura compatible. Nuestra copia x86 no sirve para esa ruta. |
| [Rinnegatamante/vitaGL](https://github.com/Rinnegatamante/vitaGL) | Capa OpenGL para Vita; candidata para render, compatibilidad a probar. |
| [vitasdk/samples](https://github.com/vitasdk/samples) | Referencia de toolchain y aplicaciones homebrew para validar ARM/VPK. |
| [SFML/SFML](https://github.com/SFML/SFML) | Biblioteca identificada por símbolos; reconstruir lógica comercial no implica decompilar bibliotecas cuyo código ya está disponible. |
| [NationalSecurityAgency/ghidra](https://github.com/NationalSecurityAgency/ghidra) | Decompilador y analizador PDB usado para las salidas locales. |
| [Vagante en Steam](https://store.steampowered.com/app/323220/Vagante/) y [Nintendo Switch](https://www.nintendo.com/es-mx/store/products/vagante-switch/) | Distribución oficial del juego. La existencia de ports oficiales no aporta su código fuente ni demuestra rendimiento en Vita. |

También se localizaron los proyectos locales `ShotKVita` y `Vita_Touhou75` como antecedentes de trabajo; sus fuentes no se incorporaron. La carpeta inicial `VaganteVita` estaba vacía. Se encontró el juego en la biblioteca Steam estándar y se analizó sin modificar su instalación.
