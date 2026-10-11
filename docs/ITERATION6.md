# Iteración 006 — icono y LiveArea

Nueva aplicación VAGV00006, separada de las anteriores. El render, resolución por L y caché del menú mantienen el comportamiento de 005. Logs: ux0:data/Vagante2Vita/logs/iteration006/session-NNNNNN.log.

## Arte incluido

Icono de espada/ramas formando una V, fondo nocturno de bosque/caravana/montañas y rótulo Vagante2Vita. Arte original generado con imagegen. Prompts y fuentes preservados en [art/livearea006](../art/livearea006/README.md).

- sce_sys/icon0.png: 128×128, burbuja de aplicación.
- sce_sys/livearea/contents/bg0.png: 840×500, fondo LiveArea.
- sce_sys/livearea/contents/startup.png: 280×158, imagen de la puerta de inicio.
- sce_sys/pic0.png: 960×544, imagen de arranque.
- template.xml: disposición psmobile, puerta a la derecha para conservar título y caravana a la izquierda.

Todos los PNG están indexados a 8 bits, con 128 colores usados y menos de 420 KB por archivo. Empaquetado siguiendo el [ejemplo oficial de VitaSDK](https://github.com/vitasdk/samples/blob/master/pretty_livearea/CMakeLists.txt) y sus [notas de imágenes](https://github.com/vitasdk/samples#notes-on-images). El pipeline se reproduce con `python tools/prepare_livearea.py art/livearea006/icon-source.png art/livearea006/background-source.png` (requiere Pillow).

## Prueba

Instalar artifacts/iteration006/Vagante2Vita-iteration006.vpk mediante VitaShell. Debe aparecer una burbuja nueva con icono, y al abrirla fondo y puerta de inicio. La entrega preserva 005. Mantener data.vra y menu-only.vra en ux0:data/Vagante2Vita/. No es necesario recopiar ni convertir recursos otra vez.

Compilación/ZIP/SFO/PNG/XML comprobados localmente. Falta comprobar en Vita la instalación, apariencia en LiveArea y arranque. Si aparece un error de instalador, anotar su código: ocurre antes de que el programa pueda crear un log de ejecución.

## Logs recibidos

El usuario reporta mejor FPS y arranque más rápido tras 005. Los archivos runtime.log y runtime.previous.log adjuntos todavía indican BOOT version=00.03 y logical=640x360, con carga de aproximadamente 44–48 segundos y presentación de alrededor de 2,6 FPS. Son registros anteriores, por lo que no cuantifican la mejora reportada. Para comparar 005/006 se necesita session-NNNNNN.log de su carpeta de iteración, con SOURCE, PROFILE y PERF de la versión correspondiente. No hay nuevo avance de gameplay ni cambio de rendimiento deliberado en 006.
