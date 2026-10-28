# Rama C: transición al nivel 2 — diagnóstico PC (2026-10-28)

## Resultado

**El bloqueo de Vita no se reprodujo en el intento normal sin pollo.** Se llegó
por teclado al piso 1, a la intermisión y al piso 2. La transición intermisión →
piso 2 terminó en **0,397753 s** dentro de GameEngine::loadNextLevel
(0,415682 s incluyendo Client::loadNextLevel). La lectura posterior identifica
floor=2, levelFloor=1, ACT_ONE=0 e intro=0. El jugador murió después por pinchos;
eso no es un atasco de carga. No se generó ni se transportó artificialmente un pollo.

Se inspeccionó la lista real de entidades: no había TYPE_CHICKEN=106 en los
pisos observados. heldUnit=0 tanto en la intermisión como tras entrar al piso 2.
**El caso específico con pollo sigue sin verificar.** Tampoco se ejecutó en Vita.

## Compilación exacta y límites

- Rama rama-c; sólo local, sin push. Build PC: --window-mib 280 (arena TLSF 248 MiB).
- Gen actual usado: 60108a83c42198c35ab3c6256a820515ec3d0e861cc56612e9ad0763381058d6 (168 unidades).
- Gen instrumentado privado: 27e40152527c69d24c46fe44d91b0112b749638f8c345ea6d48af328f13c55e6.
- Ejecutable: a96f1be95505cf11299186b9a4acb6bc82d83d475328eb1ab3993b490882e5e1.
- El runtime que se estaba editando no compiló: fast_dirty no estaba declarado.
  No se corrigió ni se sobrescribió. Se compiló la copia privada guest-head.c,
  exportada de 2f313d0c9a1dd382fb556f228301a08c2e2d9759 (HEAD al iniciar la prueba), con SHA-256
  6450b9ec4d5d03e3b52881d80324284779cc94b1abe413b8c41f750273282d6e. El resto de unidades/host usa el estado del build
  identificado arriba. La reproducción no valida los cambios posteriores del runtime.
- Datos del usuario externos; OpenAL null. No se tomaron capturas de pantalla.
- Sesión concluyente: artifacts/heap-profile/session-xsfav2_h, PID 23856,
  ejecutable privado vagante_level2_probe.exe y ventana de título exclusivo.
- Se reutilizó una copia **intacta** del savedata.sav producido normalmente por
  una sesión previa. Esto permite que el juego considere iniciado el tutorial
  (GameEngine::finishedTutorial 005ca150 consulta started_tutorial/deaths).
  No se editaron las estadísticas, vida, posición, semilla ni reglas del motor.

## Hitos ejecutados

| Paso | Estado observado | Duración de GameEngine::loadNextLevel |
|---|---|---:|
| Intro → primer piso normal | floor=1, ACT_ONE=0, levelFloor=0 | 0,386324 s |
| Piso 1 → intermisión | floor=1, INTERMISSION=9, levelFloor=1 | 0,039504 s |
| Intermisión → piso 2 | floor=2, ACT_ONE=0, levelFloor=1 | 0,397753 s |

Antes de cargar el piso 2 había 216.916.688 B vivos y 42.207.440 B libres en el
heap. Después: 219.059.424 B vivos, 40.046.608 B libres y hueco máximo de
39.936.976 B. En esta ejecución no hay evidencia de agotamiento/fragmentación
que impidiera esta transición.

Evidencia local: artifacts/level2-hang/transition-proof.txt y los live-3.json /
live-4.json de la sesión. Los JSON incluyen coordenadas y geometría privada y
se mantienen fuera de Git. Las lecturas finales usan ReadProcessMemory sin
escribir ni detener el proceso; GDB se reservó para las pilas. Los primeros
intentos sí usaron lecturas GDB, por lo que su pausa puede alterar tiempos.

Los intentos previos session-a5hzppuc, session-94bw52yi y session-bg8ayl59
terminaron con limpieza completa y ExitProcess(0), sin aviso Unresponsive ni
fallo. No se atribuye su cierre al usuario, que confirmó no haberlos cerrado;
no se identificó con evidencia la causa de esos cierres. En la sesión final
se aisló también el título para reducir interferencia con otras ventanas.

## Dos tomas de pilas: controles, NO capturas de un bloqueo

Se guardó thread apply all bt 30 para todos los hilos dos veces. El proceso se
desacopló y continuó ejecutándose entre ambas capturas. Son controles tomados
**después** de una transición que terminó; el juego ya había reiniciado la intro
tras la muerte en el segundo piso. No deben presentarse como el atasco de Vita.
Cada muestra tiene 14 bloques de hilo (incluido el hilo DbgUiRemoteBreakin que
GDB crea al adjuntarse).

- Toma 1 UTC: 2026-10-28T16:36:22.272457+00:00.
- Toma 2 UTC: 2026-10-28T16:36:33.442593+00:00.
- Pausa entre terminar la primera captura/resolución y empezar la segunda: 10.000 s.
- Diferencia entre inicios: 11.170136 s.
- Raw: artifacts/level2-hang/control-after-level2/threads-1.txt y threads-2.txt.
- Resueltas: threads-1-resolved.txt y threads-2-resolved.txt en la misma carpeta.
- capture.json conserva comandos, tiempos y retorno 0 de ambos GDB.

Los símbolos f_XXXXXXXX se resolvieron con bisect del índice privado de rama A.
Se muestra el vecino inferior y su desplazamiento; no se afirma que todo símbolo
interno tenga un nombre exacto PDB. Los módulos nativos conservan sus nombres.

| Hilo | Muestra 1 | Muestra 2 | Interpretación limitada |
|---|---|---|---|
| 1, principal | WglContext::display → SwapBuffers, driver NVIDIA | RenderTarget::clear → glClearColor → profile_context/pthread_getspecific | Cambia de trabajo; no queda en una misma espera de transición |
| 13, música | SoundStream::streamData → sleepImpl → Sleep | SoundStream::streamData → API/SetEvent | Hay avance del audio |
| 9, mezclador | Espera de evento | Mix_SSE | Hay trabajo entre capturas |
| 3, entrada | EJoystick::updateThreadLoop → sleep_for | Mismo bucle de sondeo/dormida | La repetición de una espera periódica no prueba deadlock |
| 2, perfil | profile_timer → sleep(30 s) | Misma espera intencional | Esperado por RECOMP_HEAP_PROFILE=2 |

## Clasificación solicitada

| Caso | Resultado de esta reproducción |
|---|---|
| Deadlock de secciones críticas/eventos | No demostrado: no se observó un bloqueo y las pilas de control no prueban un ciclo de espera |
| Bucle infinito del juego | No demostrado: los loadNextLevel relevantes tienen before y after y terminan |
| guest_fault perdido en otro hilo | No observado: no hay guest_fault ni retornos FA17 en el trace concluyente |

No hay MessageBoxA con Vagante Unresponsive, RaiseException(...), # catch: o
# unhandled exception en la sesión concluyente. La ausencia en PC no descarta
un error exclusivo de Vita o de la entidad transportada.

## Hallazgo estático y propuesta de arreglo (SIN APLICAR)

El aviso es del watchdog ThreadCanary, no una clasificación de su causa.
ThreadCanary::check (004e84c0) actualiza un heartbeat bajo mutex. La lambda de
vigilancia 004e82c0 compara su antigüedad y llama a MessageBoxA en 004e8387.
Se comprobó el desensamblado real: flags=0x10014, caption en 00b1a114 y cuerpo
en 00b1a130. Tras la llamada, el original distingue retornos 6 y 7. El bit de
botones es MB_YESNO=4; winuser.h local define IDYES=6, IDNO=7 e IDOK=1.

**Discrepancia concreta:** host/user.c devuelve siempre 1 (IDOK), incluso para
este cuadro Sí/No. Ese valor no entra en ninguna de las dos ramas previstas.
La rama 6 del original fuerza el fallo deliberado en BADF00D5; la rama 7 espera.

Propuesta no implementada: hacer que MessageBoxA respete el conjunto de botones
y la decisión de continuar/abortar. Para un host sin diálogo interactivo,
una política explícita de continuar usaría IDNO=7 en MB_YESNO. Registrar flags,
retorno, hilo guest y fase junto al heartbeat antes de devolverlo. Comprobar
con una prueba ABI que 0x10014 devuelve un valor válido y conserva el stdcall.
**Corregir el contrato del cuadro no resuelve por sí solo una espera de carga.**
No hay evidencia para cambiar locks o la lógica de transporte del pollo aún.

Para el caso de Vita que sí se atasca, propuesta de diagnóstico previa a un
arreglo del motor: marcar entrada/salida de Client::loadNextLevel (00550a00),
GameEngine::loadNextLevel (005c23b0), Player::setLevel (006c6c20) y los pasos
de generación/subida de texturas; registrar heldUnit (+0x474), su tipo (+0x50),
Level de jugador/unidad (+0x1a0), id del hilo y recurso de sincronización.
Conservar el watchdog y obtener la pareja de pilas cuando esté realmente detenido.
Si hay un ciclo de espera, corregir el orden/propiedad demostrado; si ambas
pilas repiten una rutina de juego sin progreso, revisar ese bucle y sus datos;
si termina un worker con FA17, propagar ese fallo al estado de carga. Estas son
acciones condicionadas a nueva evidencia, no causas identificadas en este PC.

## Cambios realizados

Sólo herramientas de diagnóstico y este documento. No se modificó runtime/guest.c,
vita/platform_vita.c, host/seh.c, host/sound.c ni host/archive.c. El runtime de
referencia exportado, stacks, mapas/estados, datos del juego y gen permanecen en
artifacts/ sin añadirse a Git. No se aplicó ninguna propuesta de arreglo.
