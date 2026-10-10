# Diagnóstico Vita

Aplicación homebrew original, sin lógica comercial. Dibuja texto con framebuffer, lee mando y busca `ux0:data/Vagante2Vita/data.vra`. Comprueba únicamente los dos bytes de encabezado zlib; no valida ni ejecuta el archivo completo. START sale de la aplicación.

Compilar con VitaSDK:

```sh
export VITASDK=/ruta/vitasdk
export PATH="$VITASDK/bin:$PATH"
cmake -S vita -B build-vita
cmake --build build-vita -j4
```

Salida: `build-vita/Vagante2Vita-diagnostic.vpk`, title ID `VAGV00001`. Copiar el archivo `data.vra` de la instalación propia a la ruta indicada para la comprobación. No es necesario copiar EXE ni PDB a Vita.

Esta aplicación sólo confirma que se puede producir un VPK ARM y prepara una prueba básica de datos/entrada/pantalla. Compilarla no demuestra que el juego sea portable o que alcance un framerate concreto. Se compiló con GCC Vita 15.2.0; ejecución en consola física pendiente.
