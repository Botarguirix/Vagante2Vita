# PaletteDatabase — rama A

2026-10-24. --unit consultado; cabeceras PDB intactas.

| Función del índice | Estado |
|---|---|
| Palette::Palette @004b7ba0 | Traducida mediante move implícito C++ de string/vector; el PDB no declara constructor explícito en la cabecera. |
| Palette::~Palette @004b79d0 | Destrucción implícita STL, fiel al cuerpo. |
| Palette::generatePaletteSwap @004b6f70 | Ya traducida en ResourceManager_images.cpp; revisada completa y probada de nuevo. |
| PaletteGroup::PaletteGroup @004b7a40 y sobrecargas | Construcción default/copy/move implícita de sus miembros STL; sin editar cabecera. |
| PaletteGroup::~PaletteGroup | Destrucción implícita STL. |
| getPaletteGroup @004b7810 | Traducida: copia del grupo o registro de error y grupo vacío. |
| getPaletteGroupList @004b7770 | Traducida: claves en orden de iteración del unordered_map. |
| setPaletteGroup @004b7db0 | Traducida: reemplazo/copia por nombre. |
| loadScripts @004b7ee0 | Traducida: palettes.json o paletteData de data.vra, limpieza e inserción de grupos. |
| saveScripts @004b7280 | Traducida: PrettyWriter indent4; comentario, grupos, texturas, paleta default primero y variantes. |
| lambda_4452ebf38f69e21726d30f632f85b45c @004b8320 | Integrada en readGroup: nombre/texturas, colores RGBA, primera paleta default, duplicados conservan primera entrada. |
| lambda_017ad50b092d12561bff3b85880ae457 @004b7630 | Integrada en writeGroup. |
| lambda_1f0770e4fc779a953f8e7d10c25df4f9 @004b74c0 | Integrada en writePalette. |

initialize y los accesores sin cuerpo independiente se recuperan del inline de
ResourceManager::runTasks: carga una vez y marca initialized incluso si falta
el archivo. Se conserva la especialización del binario a globalPaletteDb:
las operaciones de grupos apuntan al mapa absoluto 0xb93094, aunque la firma
PDB conserve this. Se usa STL normal; su orden unordered_map no está garantizado
idéntico entre MSVC y libstdc++, sin añadir ordenaciones nuevas ni RNG.

El getter existente ResourceManager::getConfig está marcado privado en la
cabecera regenerada. Se recupera acceso mediante instanciación explícita como
en Sprite.cpp, sin editar layout, firmas ni cabeceras. Para JSON inválido o
archivo no abrible se retorna sin desreferencias inválidas; estos casos podían
fallar en el original. El diagnóstico original que dice attacks.json al leer
palettes.json y el comentario del editor se conservan.

generatePaletteSwap mantiene el recorrido x exterior/y interior, comparación
solo RGB, alfa del píxel, índice resaltado Magenta y fallo parcial sin rollback.
Una paleta destino más corta puede dejar colores sin cambiar si se encuentran
en la cola del default; se conserva la regla revisada del cuerpo existente.

Verificación: build_recon_pc.ps1 -Run terminó con exit code: 0; prueba
PaletteDatabase pasó (JSON y archive, round-trip, duplicados, copia, consultas,
inicialización única, RGB/alfa, resaltado, errores y cambios parciales).
Fixtures nuevos en _local/recon-tests, sin tocar datos del juego. Logs:
_local/menus/PaletteDatabase_build.log y PaletteDatabase_test.log. No hay push
ni validación de gameplay Vita.
