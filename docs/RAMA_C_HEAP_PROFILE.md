# Diagnóstico de memoria al entrar al primer mundo (rama C)

## Uso y alcance

```powershell
python -I recomp/build_host.py --jobs 4 --window-mib 280
python -I tools/heap_profile_session.py --mode 2
# PID y carpeta de salida se imprimen; todo queda local en artifacts/heap-profile.
pwsh tools/heap_profile_key.ps1 -GameProcessId <PID> -VirtualKey 83 -HoldMs 350
python -I tools/heap_profile_report.py <sesion>/recomp-trace.log --out <sesion>/report.md
python -I tests/host/run_heap_profile_tests.py
python -I tests/host/run_heap_tests.py --stress-only
```

83 = S, confirmación del perfil predeterminado. El key.ps1 solicitado no estaba
presente bajo la rama A; el equivalente exige PID y ruta del host aislado y
comprueba el foco antes de keybd_event, manteniendo y soltando siempre la tecla.
No toma capturas. La sesión utiliza copia privada de config/custom, hardlink de
data.vra y OpenAL null; no modifica datos/partidas de la instancia del usuario.

RECOMP_HEAP_PROFILE=1 activa metadatos por bloque y snapshots de fin de carga,
entradas/salidas a newGame/loadNextLevel/swapLevel y fallo de reserva/realloc.
El valor 2 añade un hilo de reporte cada 30 segundos. Sin la variable, no hay
registros, hilo ni interceptores extra. Los interceptores ejecutan los cuerpos
originales y conservan la ABI. No se cambia qué imagen se decodifica o libera.

La ventana total es 0x11800000 (280 MiB). Con el mapa actual, la arena TLSF es
248 MiB (se restan 24 MiB iniciales y 8 MiB de VirtualAlloc). El dump imprime
window y arena por separado. Esta diferencia debe considerarse al comparar con
un log de Vita que describa el tamaño del heap de otra compilación.

## Cómo se mide

El header guest sigue midiendo 16 bytes. support.c guarda en una tabla hash del
host, protegida por el mismo heap_lock, el puntero vivo, origen y retorno directo
de la API. Se elimina al liberar y se actualiza al reasignar, incluso en sitio.
Los metadatos no consumen espacio guest; metadata_host informa su coste aparte.
Las entradas malloc/calloc/realloc y operator new/new[] guardan [esp] antes de
llamar al original; llamadas anidadas conservan la entrada exterior. HeapAlloc
conserva también su propio [esp] como heap_return_example. Las reservas directas
del adaptador de archive se etiquetan con ResourceArchive::load (0x00739180);
el origen cero identifica reservas host sin contexto guest, no una función PDB.

Cada dump recorre los bloques físicos bajo el lock, copia los datos y luego
suelta el lock antes de formatear/trazar. Da los 30 orígenes con mayor capacidad
viva, requested sin redondear, bloques, cuatro intervalos exclusivos por capacidad
(<4 KiB, 4–<64 KiB, 64 KiB–<1 MiB, >=1 MiB), suma libre y mayor hueco. Cabeceras,
capacidad viva y libre deben sumar la arena. complete/lost permiten detectar
un diagnóstico incompleto en vez de presentar un total sin advertirlo.

La tabla de sf::Image observa constructor, destructor, create, cargas, asignación
y Texture::copyToImage. El layout +0/+4 (tamaño), +8/+12/+16 (vector de píxeles)
se verificó en el pseudocódigo PDB 00817e90, 00817ec0, 008181f0, 00818340 y
0074b350. Se suma ancho*alto*4 por imagen viva con vector respaldado por un
bloque vivo del heap, y se informa también capacidad del vector. Se contempla
la alineación MSVC (puntero raw en pixels-4). Las observaciones inválidas o en
curso se cuentan aparte; un snapshot OOM puede coincidir con un create aún sin
retornar. No es una cifra de VRAM/texturas ni suma buffers PNG temporales.
Los contadores de loadImage separan cargas eager y bajo demanda exitosas; no
son equivalentes al número de imágenes vivas, pues incluyen cargas y copias
con vidas distintas. No se añaden políticas de liberación.

Los nombres se resuelven después con bisect del índice privado de rama A.
El vecino inferior no demuestra un stack completo ni incluye funciones ausentes
del índice: se muestra el desplazamiento para revisar las atribuciones.

## Coexistencia con el lifter

La primera compilación encontró unit_075.c eliminado por la regeneración paralela.
El build limitado usa una instantánea estable propia y objetos por tamaño de
ventana, sin escribir gen/, lift/ o runtime/. Si el lifter había leído la lista
natives anterior, sólo la copia privada enruta las llamadas directas de las
funciones nativas por guest_call. No cambia argumentos ni cuerpos. El hash de
la copia efectiva y el número de rutas quedan en profile-routing.json. Las
nuevas direcciones se añaden a natives.txt para regeneraciones futuras.

## Validación y resultados

Pruebas host de contabilidad: origen, cuatro intervalos exactos, realloc,
liberación/reutilización, fallo y retorno a cero: OK. Pruebas de observación RGBA,
destrucción, decodificación eager/demand exitosa y registros conservados: OK.
Estrés del heap con perfil desactivado: 4 hilos, 72.000 operaciones, sin
solapamientos/corrupción y memoria viva final cero: OK.

La evidencia de la sesión real se añadirá después de ejecutarla. No se infiere
un resultado de gameplay ni una reproducción OOM de las pruebas de fixture.
