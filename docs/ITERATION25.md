# Iteración 025: pruebas del VPK automatizadas en Vita3K

## Controles de teclado configurados

El perfil genérico del emulador usa Espacio (Cross: confirmar/saltar), Z
(Square: estocada), X (Circle: volver/reiniciar), C (Triangle: clases), Q
(L: lento), E (R: cargar arco Rogue y soltar para disparar), Enter (Start:
pausa), Retroceso (Select: ficha). Flechas para dirección, A/D también mueve
por stick izquierdo; W/S se asigna al eje vertical de ese stick, que el runtime
actual no utiliza para escalera/apuntado: usar flechas arriba/abajo para eso.
I/J/K/L: stick derecho, cuyo apuntado aún no está implementado en este port.

La herramienta tools/configure_vita3k_keyboard.ps1 aplica solo esos campos y
conserva respaldo de config.yml. No modifica privacidad, red, firmware, GPU,
guardados ni datos del usuario. El emulador debe reiniciarse para cargar el
perfil. La prueba automatizada de abajo no certifica esas pulsaciones físicas:
inyecta el estado de botones dentro del VPK, en una instancia separada.

## Entrada de pruebas optativa

Solo la presencia de ux0:data/Vagante2Vita/autotest.vts activa InputReplay.
Sin archivo, el juego lee el mando normalmente. Formato VTS1: magic de cuatro
bytes, count uint16 y reservado uint16 cero; records de ocho bytes: botones
uint32 con máscara permitida 0xf3f9, duración uint16 de 1 a 600 ticks y reservado
uint16 cero. Hasta 1024 records y 36000 ticks. Truncado, flags desconocidos y
basura final se rechazan, preservando estado. Este archivo es propio de pruebas;
no contiene recursos del juego.

El archivo válido se consume mediante rename a session-NNNNNN.log.replay en
el directorio de logs. Cada sesión conserva su secuencia; una ejecución normal
posterior no vuelve a activarla. Contador avanza con los pasos de menú/lógica,
incluidos los de pausa, y no con FPS del host. No omite estados intermedios si
un fotograma procesa varios pasos. Al terminar cierra el juego normalmente.
AUTOTEST sequence_consumed solo indica fin de secuencia, no aprobación:
las aserciones se ejecutan después sobre el log por check_vita3k_smoke.py.

Se inicializan también los sticks al centro antes de consultar el mando para
no generar movimiento a izquierda si la lectura no escribe esos campos.
No se añade panel de FPS/CPU ni telemetría de rendimiento al juego.

La integración detectó dos problemas: Vita3K podía ignorar O_EXCL y reutilizar
session-000001.log; el creador ahora consulta existencia además de conservar
O_EXCL para hardware. También hubo un fallo del emulador al liberar CDRAM antes
del cierre. Se conserva el framebuffer hasta sceKernelExitProcess; el gestor
de procesos libera esa memoria. El cierre corregido se comprueba en el log del
emulador, además de EXIT normal del juego. El validador no aprueba una excepción
host posterior, aunque la secuencia del VPK hubiera terminado correctamente.

## Lanzamiento y conservación

```powershell
./tools/test_vita3k.ps1 -Iteration 025
./tools/test_vita3k.ps1 -Action Collect -RunDirectory '<directorio devuelto por Launch>'
```

El harness toma el runtime base de _local/vita3k y usa por defecto la
instancia de pruebas _local/vita3k-vagante-clone, con `pref-path` propio,
log-level INFO, VPK instalado en ux0/app/VAGV00006, recursos externos y replay.
No usa G:\Vita3k ni cambia la partida/configuración de otro proyecto. Las
copias del firmware/runtime son privadas e ignoradas por Git.
Run espera hasta 45 segundos y recoge/verifica logs y código de salida del
proceso automáticamente. Si sigue vivo, devuelve PID sin terminarlo; Collect
permite recogerlo después. Launch inicia sin esperar, para inspección manual.
El sistema de archivos de pruebas se reutiliza sin borrar sesiones anteriores;
solo una instancia de pruebas puede usarlo simultáneamente.

Cada ejecución conserva run.json, hash del emulador/VPK, input.vts, runtime.log,
vita3k.log y result.json en _local/vita3k-vagante-clone/runs/iterationNNN/run-NNNNNN/.
Collect rechaza una ejecución todavía viva o ya recogida y exige exactamente
un nuevo session log. El VPK admite esta automatización también en Vita real
si se copia expresamente el archivo; el perfil de teclado pertenece a Vita3K.

## Pruebas y límites

Once CTests y doce pruebas Python pasan, además de probes de render/recursos.
La secuencia de 52 tramos (1321 ticks nominales) ejecutada en el VPK ARM de
Vita3K verifica menú, carga de tres clases, HP 100/80/60, movimiento, salto,
geometría de estocada en tres clases, disparo Rogue, ficha de tres páginas,
pausa/reanudación, consumo de secuencia y salida normal. Se inspeccionó además
el render del menú y mundo en el emulador. Se conservan tres ejecuciones corregidas
independientes con sus logs; la última también prueba código de proceso 0; el informe marca input_source=external replay y
physical_controller_verified=false. FPS observados por Vita3K pertenecen al
PC y no se usan para aprobar rendimiento físico de Vita.

El validador comprueba además emulator_shutdown_clean y rechaza cinco mutaciones de un log real: secuencia incompleta,
clase Mage ausente, fallo de display, ausencia de disparo y ataque bloqueado.
No aprueba por acabar el proceso o por encontrar solo BOOT. No prueba daño a
enemigos, magia, generación de piso, audio, inventario interactivo ni guardado.
La secuencia supone recursos válidos; un fallo de carga puede necesitar cerrar
la instancia de pruebas y recopilar el error. No hay watchdog destructivo.

## Entrega

VPK 025, versión 00.25, misma identidad VAGV00006. Recursos originales siguen
externos; sirve world completo de 023/024. No copiar autotest.vts a la Vita
para una partida normal. Logs de hardware separados en iteration025.
La automatización acelera las iteraciones futuras; el mundo completo sigue
pendiente de ensamblado original, transición, entidades/combate y validación
física. No se redefine el objetivo como pasar esta prueba automática.
