# Demo local ACT_ONE 01.99

La entrega está en `_local/demo-act-one-199/`. El VPK `Vagante2Vita-iteration199.vpk` usa ID `VAGV00006` y versión `01.99`. `ux0-data/Vagante2Vita/` conserva los recursos privados fuera del VPK, listos para copiar a `ux0:data/Vagante2Vita/`. El `demo-manifest.json` enumera y verifica por SHA-256 el VPK, el replay y cada recurso.

## Resolución

El render lógico predeterminado cambió de 320×180 a 640×360 y se escala a 960×540 dentro del framebuffer de 960×544. Esto calcula cuatro veces más píxeles por cuadro para mejorar la nitidez. En las pantallas de menú, L alterna entre 640×360 y el modo ligero 320×180; durante el mundo, L conserva su función de juego. El modo 640×360 no añade buffers y mantiene el mismo presupuesto de framebuffer.

## Instalar y jugar

Instala el VPK y copia el contenido de `ux0-data/Vagante2Vita/` a `ux0:data/Vagante2Vita/`. Inicia el juego, pulsa X y elige una clase. Camina a la derecha en la introducción y entra por la cueva para cargar el primer piso de ACT_ONE. Controles: cruceta o stick para caminar, X para saltar, abajo+X para bajar de plataformas, Cuadrado para melee, R para arco, SELECT para consultar el personaje y Triángulo para volver.

## Verificación y límites

El VPK recompilado conserva siete entradas permitidas (ejecutable, SFO, iconos y LiveArea) y no incorpora recursos comerciales. Pasan las 14 suites C host, 226 pruebas Python y 70 subpruebas. El test dedicado del escalador compara el mapa de píxeles de ambos perfiles, comprueba la salida 960×540 centrada en 960×544 y verifica los márgenes y el pitch de framebuffer. El paquete de campaña valida 144 archivos y el manifiesto de la demo contiene 189 archivos.

La iteración de trabajo mantiene 640×360 como resolución lógica alta y factor 3/2 hasta 960×540, centrado con dos filas negras arriba y abajo. Se extrajo el escalador a `vita/src/frame_scaler.h` y se agregó `tests/frame_scaler_test.c`. El build local de verificación está en `_local/build-resolution-check-199/Vagante2Vita-iteration199.vpk`, SHA-256 `F027C5AA21805FDE445201E2CB7DD1326F41081C56A7BFE1DAC7A565CE09E04F`. Conserva ID `VAGV00006`, versión `01.99` y las siete entradas esperadas. Aún no se ha probado esta recompilación en Vita3K ni hardware.

Vita3K 0.2.1.4194 volvió a agotar 45 segundos antes de crear la sesión nativa, usando OpenGL y una copia aislada completa de `C:\Vita3k`; sus logs de emulador y juego están vacíos. El depurador local reprodujo `0xC0000005` en SDL y Qt incluso con `--skip-title`: instrucción en `ucrtbase.dll+0x25F31`, retorno a `Vita3K.exe+0x683965`. La copia no trae PDB para resolver el nombre de función. El diagnóstico está en `_local/vita3k-c197-fullcopy-test/diagnostics/no-title-startup-20261019/`. Por tanto, el VPK compila correctamente, pero ni el nuevo modo de resolución ni la transición se han confirmado en Vita3K o hardware.

La campaña reconstruida sigue teniendo límites de combate. El controlador de la prueba ahora salta ante amenazas cercanas durante el encuentro con el Dragon del piso 0. La ruta individual vence al boss con 20/80 HP (10 HP más que la ruta previa); los pisos 1 y 2 conservan sus rutas físicas verificadas. Al encadenar los pisos, el Rogue entra al Worm con 20/80 HP y 24 flechas, pero varios contactos del Worm y un Bat lo reducen a cero en el tick 1015, antes de derrotarlo. El verificador confirmó que la traza independiente del piso 1 comenzaba con 80 HP y 30 flechas, no con el estado transferido; el planificador permite ahora establecer HP y flechas de entrada, y con 20/24 el controlador actual también muere en tick 1015. La variante de salto cada 12 ticks llegó al tick 1022, pero solo quitó 4.5 HP al Worm; se descartó. Una prueba anterior sin evasión en el Dragon entró con 10 HP y murió en tick 918. Estas trazas miden el controlador host, no una partida humana ni Vita3K. No se añade curación entre pisos sin evidencia del juego original. Varios mapas y jefes siguen siendo provisionales.

La validación más reciente de Vita3K usó una copia completa aislada de `C:\Vita3k`; el diagnóstico sin título confirmó un acceso inválido de la propia aplicación en ambos frontends. El VPK compila, pero la resolución y la transición no se han verificado dentro de Vita3K.

El build recompilado está en `_local/build-act-one-199-final/Vagante2Vita-iteration199.vpk`, SHA-256 `8e6b49ad7e0897015791286a31705384c58be600a017216f7423f327dac6d65b`. La copia de entrega `_local/demo-act-one-199/Vagante2Vita-iteration199.vpk` corresponde a ese mismo build.
