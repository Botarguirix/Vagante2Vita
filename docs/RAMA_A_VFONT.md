# VFont — tarea 6

2026-10-24, rama `rama-a`. Sin push; cabecera PDB sin cambios.

Se completan todos los cuerpos con pseudocódigo de la unidad. El parser y
CharacterInfo, recuperados en el bloque de recursos, pasan sin cambio de
comportamiento de `ResourceManager_fonts.cpp` a `VFont.cpp`; el resto de ese
archivo sigue gestionando las fuentes de ResourceManager.

## Funciones

| Función | Estado |
|---|---|
| `VFont::loadFromImage` @007ebd30 | Traducida; parser trasladado desde el bloque 7b, contrastado de nuevo con Ghidra y argumentos de getPixel. |
| `VFont::getStringWidth` @007ebfb0 | Traducida: bytes unsigned, suma de anchos, dos píxeles menos por carácter si el bool está activo. |
| `VFont::drawString` @007ec020 | Traducida: textura diferida, color, posición, rect por carácter, avance especial del espacio y ajuste de ancho/bias. |
| `VFont::CharacterInfo::CharacterInfo` | Reconstruida de los valores inline: `(x,y,w,h)=(0,0,8,1)`; trasladada del bloque 7b. |
| `VFont::loadFromTexture` | Sin cuerpo independiente en el índice; wrapper inferido sobre `loadFromImage` y `copyToImage`, guarda el puntero sin apropiarse de él. Null deja la fuente sin cargar. |
| `VFont::getStringWidthWithoutSpaces` | Sin cuerpo independiente en el índice; wrapper inferido que elimina espacios ASCII y usa medición normal. |
| `VFont::getNumSpaces` | Sin cuerpo independiente en el índice; cuenta espacios ASCII. |

No se añade un destructor: el contrato no lo declara y la destrucción STL es
implícita. El `scalar deleting destructor` que Ghidra atribuye a ResourceManager
es generado por el compilador y no se reconstruye.

## Constantes y detalles conservados

- 128 CharacterInfo, empieza por ASCII 32 y termina al llegar a 123 o agotar las
  filas. Los marcadores rojo/azul delimitan glifos; se resta 4 al alto final.
- La medición ignora `wBias` y acepta espacios como cualquier otro carácter.
  El dibujo sí suma `wBias` para glifos distintos de espacio; el espacio avanza
  exactamente el tercer float del método. Ese float no es una escala de fuente.
- El bool del dibujo reduce el avance no espacial en 2 píxeles; no recorta el
  rectángulo dibujado. La medición ajustada también reduce el espacio en 2.
- `ResourceManager::getTexture` recibe `false`: `xor cl,cl` en @007ec08d,
  verificado con `tools/call_args.py`.
- La textura se guarda para siguientes dibujos. El parser no borra una textura
  previamente cacheada al recargar, igual que el cuerpo recuperado.
- Los wrappers no indexados se distinguen expresamente como inferencias; no hay
  anomalías de firma que requieran modificar el contrato PDB.

## Adaptaciones

El parser mantiene las comprobaciones de bordes incorporadas en 7b para rechazar
imágenes corruptas sin leer fuera de sf::Image. El dibujado usa bytes unsigned,
coherentes con la medición, y omite índices fuera del vector: el original usa
char con signo en este punto y podría indexar antes del vector para bytes >=128.
No se intenta reproducir ese acceso indefinido ni se añade traducción Unicode.
No hay llamadas al RNG ni APIs Windows/Steam.

## Verificación

- `pwsh tools/build_recon_pc.ps1 -Run`: regenera, compila y enlaza, abre la
  ventana nativa y termina al cerrarla normalmente con **exit code: 0**.
  `recon.log`: 55 funciones pendientes alcanzadas, ninguna de VFont.
  Log: `_local/sprite-vfont/vfont-build-run.log`.
- `pwsh tools/run_recon_tests.ps1 -Unit VFont`: pasó el parser de varias filas,
  medidas, espacios, ajuste, bias, carga diferida, acceso por textura, rechazo
  de atlas sin marcadores/carácter abierto, tinte y posiciones de glifos por
  comparación de píxeles de un RenderTexture SFML. Bytes fuera de rango se omiten.
  Log: `_local/sprite-vfont/vfont-test.log`.
- `pwsh tools/run_recon_tests.ps1 -Unit ResourceManagerLoading`, con el archivo
  del usuario: pasó otra vez **2730/2730**, incluidas las tres fuentes bitmap,
  sus glifos y el resto de imágenes/audio/fuentes. Log:
  `_local/sprite-vfont/vfont-loading-regression.log`.
- Atlas y captura sintéticos:
  `_local/recon-tests/VFont/work/vfont-fixture.png`; no se incluyen datos
  comerciales en Git. No se genera VPK ni se valida gameplay en Vita.
