# MainMenu — rama A

2026-10-24. --unit MainMenu consultado; cabeceras intactas.

| Función del índice | Estado |
|---|---|
| MainMenu @006a2ca0 | Traducida: referencias, vista, sprites, 9 doodads, 4 botones y reset. |
| ~MainMenu | Traducida: delete de elementos y sprites de doodads; resto por STL. |
| updateStrings | Traducida: New Game/Play Online/Options/Exit Game con localización. |
| reset @006a6550 | Traducida: flags, música, tween de caravana y variante sin intro. |
| up @006a6790 | Traducida: un evento por frame, foco y límite inferior. |
| down @006a67d0 | Traducida: un evento por frame, foco y límite superior3. |
| action @006a6810 | Traducida: saltar intro o propagar pulsar/soltar al botón seleccionado. |
| skip @006a6720 | Traducida: ready, caravana x50/y232. |
| stepBackground @006a3ff0 | Traducida: movimiento, animación, RNG y seno/coseno de doodads. |
| step | Traducida: input, título, tween, hover, botones, cursor y estados de salida. |
| drawBackground @006a4ba0 | Traducida: orden de capas, parallax, día/noche, fireflies y caravana. |
| draw @006a5b50 | Traducida: fondo, título/sombra/glow, elementos y fades. |

Constantes recuperadas: FADE_OUT_TIME_GAME=90, FADE_OUT_TIME_NET_MENU=40,
FADE_IN_TIME=30, INTRO_END=400; tween de llegada dura520. El nombre
INTRO_CARAVAN_ENTRANCE no se puede asociar inequívocamente a un inmediato y
conserva su TODO. Los estados anónimos son 0=start,1=net,2=progression,3=credits.
Se conserva la construcción de solo cuatro botones; unlocks/credits no se
construyen en este cuerpo aunque haya ramas que los comparan. Se inicializan
esos dos punteros y caravanBump a cero para evitar lecturas indeterminadas.

Diferencias de Ghidra contrastadas con instrucciones:
- draw ignora el float de su firma; day=1, salvo start fade, donde usa
  clamp(animateOutTimer/animateOutTime,0,1). No usa el cookie como argumento.
- La función compartida @00814110 con ECX=&view devuelve &view.size; se usa
  view.getSize(), no el RenderTarget::getDefaultView mal identificado.
- RNG de caravana: exactamente un MT19937 &0x7fffffff y modulo20 cuando
  abs((rate*0.3-0.3)-rate)>0.001. Constructor/draw no consumen RNG.
- Tween guarda c como desplazamiento: reset pasa targetCaravanPosition-b.
- Se recuperaron los argumentos perdidos de música, sprites y fmod mediante
  call_args.py, asm y PE.cstring. Se conserva el orden del recorrido de capas.

Wrappers simples de flags/getCaravanSprite se incluyen para uso de llamadores.
mouseAction, startCredits y resized no tienen cuerpo independiente en índice;
permanecen stubs. GuiCursor y NetMenu son dependencias pendientes; sus llamadas
se conservan mediante las APIs públicas, sin ampliar estas unidades ni añadir
Steam. Esto no declara completo el flujo global del juego.

Verificación: el build en la carpeta principal encontró un Client.cpp nuevo,
ajeno a esta tarea, con accesos privados y constructor Tween inexistente. No se
modificó ese archivo. Se preparó _local/menus/validation con copias idénticas de
nuestras fuentes, cabeceras y dependencias; excluye Client.cpp y regenera su
stub anterior. data.vra es un hardlink local del archivo del usuario, sin Git.

En esa copia: pwsh tools/build_recon_pc.ps1 -Run compiló y cerró normalmente con
exit code: 0; run_recon_tests.ps1 -Unit MainMenu pasó (construcción/destrucción,
intro, navegación, cuatro destinos de fade, eventos, RNG exacto, capas/título y
float ignorado). También pasó de nuevo Tween tras corregir su constructor.
Logs: _local/menus/MainMenu_isolated_build.log, MainMenu_test.log y
Tween_recheck.log. El primer arranque de la copia no tenía data.vra y falló;
se repitió con los datos presentes. Sigue pendiente el build integrado mientras
Client.cpp esté en curso. Sin push y sin validación de gameplay/Vita.
