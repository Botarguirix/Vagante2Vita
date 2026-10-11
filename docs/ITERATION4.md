> Política histórica de IDs independientes, sustituida desde [007](ITERATION7.md): VAGV00006 permanece fijo. Las carpetas y logs continúan separados.

# Iteración 004: preservar versiones y sesiones

Las entregas se guardan en `artifacts/iterationNNN/`, sin reemplazar carpetas existentes. El empaquetador rechaza una iteración ya guardada. Cada entrega nueva conserva VPK, snapshot ZIP del código de su commit, roadmap, log de compilación y hashes SHA-256. Las iteraciones históricas 001–003 conservan los VPK existentes y sus hashes; no se afirma que tengan snapshots de código asociados a esta nueva entrega.

00.04 usa el identificador independiente **VAGV00004**. Puede instalarse junto a la aplicación anterior VAGV00001. Las iteraciones futuras deben tener identificador y carpeta propios. Los VPK históricos 001–003 compartían identificador: sus archivos están preservados, pero instalarlos sustituye la aplicación histórica con ese identificador.

La lógica y vista de clases son las de 00.03. Cambia la conservación de logs y se añaden errores de inicialización del framebuffer/display. Mantener `data.vra` en `ux0:data/Vagante2Vita/data.vra`; compartir ese archivo evita duplicar recursos comerciales.

## Logs en Vita

Cada arranque de 004 genera un archivo nuevo:

`ux0:data/Vagante2Vita/logs/iteration004/session-000001.log`

Luego `session-000002.log`, etc. La creación usa exclusividad: un archivo existente jamás se trunca ni se reutiliza. No se borran sesiones antiguas. Los antiguos `runtime.log` y `runtime.previous.log` también permanecen intactos. Si la carpeta no se puede crear o escribir, aparece un aviso en pantalla; ese fallo no puede registrarse en el archivo que no se pudo abrir.

Enviar el log de la última sesión tras probar. Contiene BOOT, RESOURCE, ARCHIVE, LOAD, EVENT, PERF, BLOCKER, ERROR/WARN y EXIT como se describe en ITERATION3.md. Un cierre anormal puede dejar el log sin EXIT; las líneas previas se vacían periódicamente y en transiciones.

## Producir una entrega

Compilar en una carpeta independiente, por ejemplo `build-iteration004`, y guardar la salida en `build-iteration004/build.log`. Guardar primero el código en un commit; después:

`python tools/package_iteration.py 004 build-iteration004/Vagante2Vita-iteration004.vpk --build-log build-iteration004/build.log`

En Windows se puede especificar la ruta del binario con `--git`. El paquete es local y está excluido de Git. `source.zip` sólo contiene archivos versionados: excluye recursos, pseudocódigo y archivos comerciales locales.
