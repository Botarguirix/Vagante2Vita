# Demo ACT_ONE local — VPK 02.02

El paquete está en `_local/demo-act-one-202/`. `Vagante2Vita-iteration202.vpk`
instala sobre el mismo título `VAGV00006`; `demo-manifest.json` contiene el
SHA-256 de todos los archivos.

Instala el VPK y copia el contenido de `ux0-data/Vagante2Vita/` a
`ux0:data/Vagante2Vita/`. Inicia el juego, pulsa X y elige una clase. Camina a
la derecha y entra por la cueva para cargar el primer piso de ACT_ONE. Controles:
cruceta o stick para caminar, X para saltar, abajo+X para bajar de plataformas,
Cuadrado para melee, R para arco, SELECT para consultar personaje y Triángulo
para volver.

La salida usa los 960×540 píxeles visibles del framebuffer de 960×544. El modo
alto presenta 640×360 con conversión 3:2 sin filas omitidas; L alterna entre ese
modo y 320×180 en los menús. La verificación de host pasa, pero el VPK todavía
no ha iniciado en Vita3K ni en hardware. En esta máquina Vita3K 0.2.1.4194
termina con `0xC0000005` antes de cargar títulos.

La demo conserva la limitación de campaña documentada en
[iteración 201](ITERATION201.md): el controlador host puede completar cada
piso por separado, pero la cadena de salud persistente no completa el piso 1.
