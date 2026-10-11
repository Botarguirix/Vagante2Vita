> Versión histórica: el usuario confirmó que cargó en Vita. La siguiente prueba es [00.03](ITERATION3.md).

# Prueba del título 00.02

Instalar build-vita/Vagante2Vita-title.vpk con VitaShell. Usa VAGV00001 y actualiza el diagnóstico anterior. Mantener tu data.vra completo en ux0:data/Vagante2Vita/data.vra. El VPK no contiene recursos comerciales. El arranque recorre todo el archivo: esperar en LOADING AND VERIFYING DATA VRA.

- Cruceta arriba/abajo: seleccionar.
- X: aceptar; las funciones pendientes muestran un aviso.
- Círculo: volver desde el aviso.
- Exit Game o START + SELECT: salir.
- Si falla la carga, START sale. Consultar ux0:data/Vagante2Vita/runtime.log.

En PC se verificaron todos los registros y diez texturas. Falta validar esta versión en consola. El título es aproximado, sin animaciones, audio ni partida. render_us incluye espera/presentación: no indica FPS. Si falla antes de abrir el registro, éste puede no existir.

## Compilar

Con VitaSDK: cmake -S vita -B build-vita -DCMAKE_BUILD_TYPE=Release y cmake --build build-vita.

Escritorio: cmake -S vita -B build-host -DHOST_PROBE=ON -DCMAKE_BUILD_TYPE=Release y cmake --build build-host. Ejecutar build-host/vagante-probe RUTA_DATA_VRA preview.ppm. Retorna 1 con archivo inválido y 0 tras verificar y dibujar fondo/logotipo. Interfaz y controles se prueban en Vita.
