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

El origen/retorno por hilo usa pthread_key_create/pthread_getspecific con
liberación al terminar el hilo. No usa __thread/_Thread_local, compartidos en
Vita. La prueba de cuatro hilos retiene 64 bloques por origen y comprueba que
ninguno se atribuya a otro hilo.

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
y Texture::copyToImage. También observa ImageResource::ImageResource
(0074b2c0), cuyo constructor copia la sf::Image inline: sin él se omitirían
máscaras/outline y se subestimaría la memoria de imágenes. El layout +0/+4 (tamaño), +8/+12/+16 (vector de píxeles)
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

Sesión definitiva: `artifacts/heap-profile/session-fqr6_4gl` (PID 14600).
Se ejecutó el juego real con modo 2, ventana 280 MiB y arena TLSF 248 MiB.
El índice y los datos permanecieron privados. No se tomó ninguna captura.
La ejecución usó el gen actual regenerado con excepciones:
`2d2f4d8835a25965716c47d72156c4c13c7a85693478c4219e49c04387f0753a` (168 unidades, incluye seh_resume_map).
Copia instrumentada efectiva: `8618fd2fe781d9edc3c05b689f993149b7503b4405e89ca66bf53f14c7fe6636` (3039 rutas nativas).
Host medido: `c994265eb63453346323774f8d01061afa17c417bb25929d150f1983ce5ed525`.
Datos SHA-256: `3ff6d1a4d17ff3c0f7865d328c916887dc0f9030cd60fdcfdcd7e455f7b9958f`.

Pruebas host: contabilidad por origen/intervalos, realloc, OOM de fixture,
cuatro hilos con orígenes distintos, destrucción y copias ImageResource: OK.
Estrés con perfil desactivado: 4 × 18.000 operaciones, datos y no solapamiento,
memoria viva final cero: OK. El fallo del fixture no se presenta como OOM del juego.

El juego pasó por MainMenu, elección del Knight (clase 0), aparición del jugador
y loadNextLevel. La evidencia final es floor=1, intro=0, levelIntro=0, y la línea
playMusic(caves_theme.ogg). S confirma el menú principal; en este perfil
ACTION_MENU es Space, que abre y confirma la clase; S en el selector no basta
para finalizar la clase. Se usó keybd_event con scancode MapVirtualKey y teclas
mantenidas, siempre contra el PID de prueba. La sesión inicial se cerró desde
su propia ventana; nunca se cerraron procesos del usuario.

### Memoria observada (bytes, capacidad viva del heap)

| Hito | Heap vivo | Libre | Mayor hueco | RGBA vivo | Imágenes |
|---|---:|---:|---:|---:|---:|
| Fin de carga | 224,638,896 | 34,712,272 | 33,744,416 | 101,176,332 | 1,198 |
| Client newGame antes | 244,137,136 | 15,023,520 | 14,815,360 | 116,048,580 | 1,346 |
| GameEngine newGame antes | 244,137,312 | 15,023,200 | 14,815,360 | 116,048,580 | 1,346 |
| GameEngine newGame después | 245,704,624 | 13,442,384 | 13,410,288 | 116,810,948 | 1,360 |
| Client newGame después | 245,704,592 | 13,442,432 | 13,410,288 | 116,810,948 | 1,360 |
| Client loadNextLevel antes | 251,114,752 | 7,999,936 | 7,862,560 | 121,899,716 | 1,500 |
| GameEngine loadNextLevel antes | 251,114,752 | 7,999,936 | 7,862,560 | 121,899,716 | 1,500 |
| GameEngine loadNextLevel después | 254,022,064 | 5,051,584 | 4,803,200 | 124,300,356 | 1,556 |
| Client loadNextLevel después | 254,022,064 | 5,051,584 | 4,803,200 | 124,300,356 | 1,556 |
| Mundo, +80,36 s | 257,883,968 | 1,125,648 | 861,872 | 127,011,908 | 1,574 |

Todos los volcados completos indican complete=1, lost=0 e
invalid_or_inflight=0. En todos, vivo + libre + cabeceras = arena;
los cuatro intervalos suman los bytes vivos. metadata_host es sólo la tabla
hash persistente del profiler (1.572.864 → 3.145.728 B), fuera de la ventana
guest; no incluye el snapshot temporal, registros de imágenes ni contextos pthread.

El contador definitivo incluye 0074b2c0 ImageResource::ImageResource: sus
copias inline no pasan por el constructor sf::Image normal. Las sesiones
anteriores `session-3isx855n` y `session-dpmdmd1i` omitían esas copias en el
contador RGBA; se conservan para diagnóstico y **no** sustentan la cifra final.
No se modificó cuándo se decodifican/liberan recursos ni se eliminaron copias.

### Los 15 mayores orígenes (+80,36 s dentro del mundo)

| Origen | Función según PDB | Bytes vivos | Bloques |
|---|---|---:|---:|
| 0x0040b1fa | std::allocator<char>::allocate +0x1a (inicio 0x0040b1e0; vecino inferior) | 126,709,408 | 1310 |
| 0x00739180 | ResourceArchive::load +0x0 (inicio 0x00739180; vecino inferior) | 71,797,648 | 9077 |
| 0x00837203 | std::allocator<unsigned_short>::allocate +0x23 (inicio 0x008371e0; vecino inferior) | 40,757,344 | 304 |
| 0x00754a8e | Room::Room +0x21e (inicio 0x00754870; vecino inferior) | 4,087,216 | 8534 |
| 0x004139b4 | std::allocator<AttackScriptDatabase::AttackScriptKeyframe>::allocate +0x24 (inicio 0x00413990; vecino inferior) | 2,489,344 | 32 |
| 0x00648716 | Level::step +0x1e6 (inicio 0x00648530; vecino inferior) | 1,228,832 | 3840 |
| 0x0045bf77 | std::allocator<sf::Vertex>::allocate +0x27 (inicio 0x0045bf50; vecino inferior) | 777,680 | 29 |
| 0x00757b20 | Room::load +0x150 (inicio 0x007579d0; vecino inferior) | 705,632 | 986 |
| 0x0098a43b | FUN_0098a430 +0xb (inicio 0x0098a430; vecino inferior) | 651,424 | 437 |
| 0x0040b21a | std::allocator<char>::allocate +0x3a (inicio 0x0040b1e0; vecino inferior) | 541,376 | 4615 |
| 0x009119b1 | __book_unquantize +0xb1 (inicio 0x00911900; vecino inferior) | 524,816 | 416 |
| 0x00921ff0 | ogg_stream_init +0x30 (inicio 0x00921fc0; vecino inferior) | 524,288 | 32 |
| 0x0091049a | FUN_00910260 +0x23a (inicio 0x00910260; vecino inferior) | 524,288 | 64 |
| 0x0091147f | vorbis_book_init_decode +0x14f (inicio 0x00911330; vecino inferior) | 428,384 | 1248 |
| 0x009114f5 | vorbis_book_init_decode +0x1c5 (inicio 0x00911330; vecino inferior) | 428,272 | 1248 |

[Volcado completo, 30 orígenes](C:/Users/Andy PC/Documents/Chatgpt/Vagante2Vita-ramaC/artifacts/heap-profile/session-fqr6_4gl/dumps/10-periodic-30s.log)

El origen char es genérico: no se equipara por sí solo a imágenes. El contador
separado demuestra 127.011.908 B RGBA (121,13 MiB) en 1.574 imágenes, frente a
101.176.332 B en 1.198 imágenes al terminar la carga. ResourceArchive::load
atribuye unos 71,8 MB vivos a sus datos/metadatos retenidos; no todo ese grupo
se identifica como PNG. El origen allocator<unsigned_short> conserva unos
40,76 MB; el nombre genérico no basta para asignar cada bloque a un recurso.
Las direcciones se resolvieron por vecino inferior del índice privado; FUN_
significa nombre generado sin símbolo PDB resuelto. No se inventaron nombres.

### OOM, excepciones y terminación

No hubo fallo heap_alloc/heap_realloc en esta sesión PC: **no existe un volcado
OOM real** y no se provocó una reserva artificial para producirlo.
La primera entrada terminó a 69,694785 s; el último periódico a 150,054563 s:
**80.359778 s dentro del primer mundo** sin OOM. Quedaban sólo 1.125.648 B
libres y el hueco mayor era 861.872 B: hay poco margen, pero esta corrida no
reproduce el hueco de 523 KiB observado en Vita. No demuestra que la Vita esté
resuelta ni que otra semilla/recorrido en PC no falle.

No aparecen líneas RaiseException(...), # catch: o # unhandled exception.
El host terminó después, a ~166,383 s totales (~96,69 s tras entrar), por:

```text
# vagante host: guest_fault at 0x009afe51: unsupported: pmaxsw xmm0, xmm3, exit code 0000fa17, 22595562 Win32 calls in 166383 ms, heap 251889 KiB (trace: recomp-trace.log)
```

Es un guest_fault por una instrucción SSE no soportada, **no** un error de
memoria. No se tocó seh.c, kernel32.c, natives.txt, lifter.py, lift_all.py,
analysis/eh.py ni runtime/guest.* después del commit de excepciones del usuario.

Informe completo con 15 orígenes por hito, histogramas y enlaces a los 30
orígenes raw: `artifacts/heap-profile/session-fqr6_4gl/report.md`.
Los diez volcados seleccionados están en `dumps/`; el trace completo mantiene
todos los periódicos de 30 segundos. session.json identifica ejecutable, gen y
datos. Artifacts, datos comerciales y fuentes generadas no se añaden al commit.
