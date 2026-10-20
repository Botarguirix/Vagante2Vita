# Demo ACT_ONE local — VPK 02.03

El paquete instalable está en `_local/demo-act-one-203/` y el VPK compilado en
`_local/build-act-one-203/Vagante2Vita-iteration203.vpk`. El manifiesto
`demo-manifest.json` verifica 189 archivos, incluidos el paquete y los recursos
que deben copiarse a `ux0:data/Vagante2Vita/`.

Instala el VPK sobre el título existente `VAGV00006`, copia el directorio
`ux0-data/Vagante2Vita/` a `ux0:data/Vagante2Vita/`, inicia el juego, pulsa X y
elige clase. Camina a la derecha y entra en la cueva para cargar ACT_ONE. La
cruceta o stick mueve, X salta, abajo+X baja de plataformas, Cuadrado ataca, R
usa el arco, SELECT muestra el personaje y Triángulo vuelve.

La zona visible de salida es 960×540 dentro del framebuffer 960×544. L cambia
al modo ligero 320×180 en los menús. El impacto ahora muestra un destello rojo
de 10 ticks; no modifica daño ni movimiento. El test de Vita3K no está validado:
la versión 0.2.1.4194 falla con `0xC0000005` antes de cargar títulos. La cadena
de campaña persistente todavía puede morir en el piso 1.
