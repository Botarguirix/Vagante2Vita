# Rama C: build nativo para PS Vita

Pipeline local de recompilación estática x86 → C → ARM Cortex-A9. La referencia
Touhou_123/Touhou123Vita/recomp/build_vita.py y sus archivos recomp/vita se
consultaron sólo para el método de compilación, plataforma y empaquetado. No se
modificó Touhou ni la carpeta rama A. Los archivos reservados del lifter, runtime,
window.c, kernel32, threads.c y main.c sólo se leen/compilan.

## Construcción

Desde la carpeta Vagante2Vita-ramaC:

```powershell
python -I recomp/build_vita.py
# Opciones: --jobs 8, --opt=-Os (predeterminado), --opt=-O1, --opt=-O2, --trace-calls
# Diagnóstico rápido de portabilidad, sin compilar el juego ni enlazar:
python -I recomp/build_vita.py --host-only
```

El SDK predeterminado es C:\vitasdk-pm (se puede cambiar con VAGANTE_VITASDK).
No toma el VITASDK global, que en este equipo apunta a otra instalación.
Se observó GCC ARM 15.2.0, SDK construido el 2026-08-25; paquetes locales
sdl2_vitagl 2.32.8-1, vitaGL r1488.g2bdbe89 y openal-soft 1.19.1-2.
Las bibliotecas se verificaron contra los headers/pkg-config instalados y las
listas de enlace de tools/build_recon_vita.sh/recon/sfml-sdl/CMakeLists.txt.

Entradas: artifacts/recomp/9af1d8cc204aca2be94f597af21ced50d77d09c1ff8f998def5f2920c3c4624c/gen/unit_*.c,
table.c y lifted.h, recomp/runtime/guest.c y recomp/host/*.c. Se excluye
platform_pc.c y se incluyen los archivos de recomp/vita. No se omitieron funciones
del juego para reducir el tamaño.

Compilación: GNU C11, Thumb/ARMv7 Cortex-A9, NEON hard-float, -Os, sin fast-math,
-ffp-contract=off, -frounding-math y -fno-strict-aliasing. GUEST_WINDOW se fija en
0x10000000 (256 MiB). SDL2, OpenAL, vitaGL, sus dependencias y los stubs Vita se
enlazan en grupo. El enlace usa response file para evitar el límite de longitud
de comandos de Windows con rutas largas y espacios.

## Incremental y trabajo paralelo

Hay hasta ocho compiladores por defecto. Cada objeto conserva una firma del
comando y del compilador; se recompila si cambian fuente, cabeceras del proyecto,
flags o SDK. Optimizaciones y modo trace usan cachés independientes. Los nuevos
objetos se escriben en temporales y se reemplazan sólo después de compilar bien.
Un fallo no deja un objeto truncado aceptable para la caché.

Se observó otra ejecución de lift_all.py regenerando y borrando unit_*.c durante
el primer intento ARM. Ahora el build espera una colección estable y copia
unit_*.c/table.c/lifted.h a vita/gen-snapshot. Verifica nombres, tamaños y mtimes
antes/después; conserva mtimes de contenido idéntico y registra SHA-256 del
conjunto. Compila esa copia sin bloquear ni cambiar el lifter. El informe de
build identifica el conjunto exacto usado; no se afirma que sea la última
regeneración si el otro proceso vuelve a modificarlo después.

Los includes locales del host/runtime se detectan como dependencias. Después de
actualizar headers o bibliotecas del SDK conviene usar otra carpeta SDK o borrar
la caché concreta de objetos para recompilarla; no se vigila cada archivo interno
de una biblioteca estática.

## Plataforma y compatibilidad

platform_vita.c muestra un log de diagnóstico de Vagante y conserva heap newlib
de 32 MiB; main.c existente fija la pila principal en 8 MiB. Los datos externos y
los informes van en ux0:data/Vagante. Se usa ATTRIBUTE2=12 para solicitar hasta
365 MiB MAIN y SELF sin la opción safe, igual que el build rama A.

VitaSDK no tiene GL/gl.h: recomp/vita/include/GL/gl.h adapta la cabecera a vitaGL.
La implementación de glGetTexImage usa un framebuffer temporal para las texturas
2D, nivel cero, RGBA/UNSIGNED_BYTE que pide SFML copyToImage. Registra dimensiones
al cargar/recrear texturas, las elimina al borrarlas y restaura el framebuffer
anterior después de leer. Casos no soportados fallan explícitamente mediante
guest_fault con el contexto CPU del llamador; no devuelven datos ficticios.
Esta ruta se verificó en PC con OpenGL real; aún falta probarla con vitaGL real.
El stub de red select se renombró internamente para no chocar con newlib;
el nombre del import guest sigue siendo select y la red sigue desactivada.

XInput llega a los mandos por SDL_GameController. El mapeo es Xbox estándar:
cruceta, Cross→A, Circle→B, Square→X, Triangle→Y, L/R→LB/RB, Select/Start→Back/Start
en el backend SDL de Vita. Los sticks mantienen ambos ejes y Y positiva hacia
arriba. L2/R2/L3/R3 sólo tendrán valores cuando el dispositivo/backend los ofrezca;
no se inventaron combinaciones de botones para sustituirlos.

## Paquete y datos externos

Salida prevista en artifacts/recomp/<sha>/vita/Vagante-ramaC.vpk, TITLE_ID
VAGA00002, APP_VER 00.01. No pisa VAGA00001 de rama A.
El VPK se construye con una lista cerrada: eboot.bin, param.sfo, LiveArea y una
fuente bitmap propia para el log. Se generan gráficos de diagnóstico propios.
No contiene vagante.exe, data.vra, mapas, sprites ni otros recursos del juego.

Para una futura prueba, el usuario copia su propia versión Steam 1.10.1:

```text
ux0:data/Vagante/vagante.exe
ux0:data/Vagante/data.vra
```

El SHA esperado del ejecutable es
9af1d8cc204aca2be94f597af21ced50d77d09c1ff8f998def5f2920c3c4624c.
La ruta vitaGL requiere el módulo del usuario libshacccg.suprx en ur0:data;
no se distribuye dentro del paquete. Recoger recomp-trace.log,
recomp-result.txt y, si la plataforma lo genera, recomp-screen.bmp de
ux0:data/Vagante. La prueba física debe comprobar arranque, apertura de ventana,
mandos, carga de recursos y consumo de memoria antes de intentar gameplay.

## Pruebas ejecutables

```powershell
python -I tests/host/run_tests.py stubs xinput
python -I tests/host/run_tests.py vita_gl_compat
python -I tests/host/vita_build_test.py
```

XInput: cuatro mandos virtuales SDL, bits, gatillos, sticks, paquetes y hotplug.
Compatibilidad GL: lectura por framebuffer real comparada con glGetTexImage de
OpenGL PC, redimensión, restauración del binding, límites y fallos explícitos.
Build: seis pruebas de caché, interrupción, cambios concurrentes, medición exacta
.text y lista del VPK que excluye ejecutable/datos incluso si existen al lado.

## Medida y validación

La cifra .text se obtiene de arm-vita-eabi-size -A (sección exacta, no el agregado
Berkeley que incluye rodata). eboot.bin se mide después de vita-make-fself; con
-c está comprimido, por lo que su tamaño en disco no es la RAM del ejecutable.
El JSON build-metrics.json registra secciones, tiempos, flags, snapshot y hash VPK.

La viabilidad de memoria depende también de rodata/data/bss, alineación, SDL,
OpenAL y vitaGL. Sólo guest + heap + pila ya suman 296 MiB; compilar/enlazar y
medir no valida que el resto quepa ni que el juego arranque en hardware.
La ventana guest de 256 MiB deja 184 MiB para su heap, 48 MiB para VirtualAlloc
y 24 MiB para imagen, pilas, TLS y huecos. El presupuesto y la precisión x87
sobre ARM aún requieren observación; no se cambiaron las reglas del runtime.

Resultado final observado (build ARM, sin prueba de gameplay):

| Medida | Bytes | MiB |
|---|---:|---:|
| .text ARM | 23,095,184 | 22.03 |
| .rodata | 1,058,452 | 1.01 |
| .data | 15,902 | 0.02 |
| .bss | 2,521,404 | 2.40 |
| eboot.bin comprimido | 15,967,739 | 15.23 |
| VPK | 15,888,198 | 15.15 |

Se compilaron 168 unidades generadas y 183 objetos totales. La primera
compilación completa duró unos 1154 s; repetir el build final recompiló sólo
2 objetos, ninguna unidad generada, y terminó en 29.9 s. El enlace final
incluye pthread con whole-archive, como rama A.

SHA-256 del VPK:
`1b24da9521dde82ce7d067678a308dd47fdfaa6ecca1910a8c20bc84ea5bd628`.

SHA-256 de eboot.bin:
`88de9aed6c12ae52a747dec7bec574554e753254d0410b1099ca08f861bb3cfe`.

SHA-256 de la instantánea C:
`697d299b40fb100272e05a36b3c437d15d7981a19679617ca12676f2994fadfd`.

check_vpk.py de vita-port verificó 7 entradas, TITLE_ID VAGA00002, APP_VER 00.01,
CATEGORY gd, ATTRIBUTE2=12, SELF y LiveArea: cero errores y cero advertencias.
Se verificaron CRC y lista cerrada del ZIP. Todas las pruebas host de APIs,
XInput, compatibilidad GL y las seis pruebas Python terminaron bien.
Evidencia local en artifacts/: vita-build-final.log, vita-vpk-check.log,
host-final-tests.log, vita-gl-final-tests.log y vita-build-tests.log; las medidas
estructuradas están junto al VPK en build-metrics.json.

La suma de .text/.rodata/.data/.bss es 25.45 MiB. Con guest + heap +
pila, la base calculada es 321.45 MiB, antes de alineación, otras
secciones y asignaciones dinámicas de SDL/OpenAL/vitaGL. El tamaño del código no
bloquea por sí solo la rama C, pero no demuestra que el juego quepa al cargar
recursos ni que rinda bien. La ventana PC actual es mayor que la ventana de
256 MiB de este VPK. Falta probar arranque, memoria, entrada, audio y render en
Vita física; no se lanzó Vita3K ni se presenta como gameplay validado.

Coordinación: el commit concurrente 908d389 incorporó los archivos de esta tarea
que estaban preparados en el índice junto a los cambios PC del otro trabajo.
Se conservó ese commit sin modificar su historial. Este cierre documenta las
medidas y pruebas finales en un commit local separado. No se editaron los
archivos reservados del lifter/runtime/window/kernel32/threads/main.
