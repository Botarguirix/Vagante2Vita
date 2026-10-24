# Credits — rama A

2026-10-24. --unit Credits consultado; cabeceras PDB intactas.

| Función del índice | Estado |
|---|---|
| Credits::Credits | Traducida: referencias, vista, nueve sprites/orígenes, color y triggered=false. Los escalares que inicializa el trigger no se inventan en el constructor. |
| EnemyParadeEntry::~EnemyParadeEntry | Destrucción implícita C++ del vector y weak_ptr. |
| PlayerSpriteCollection::operator= | Asignación implícita C++ por miembros, incluidas las operaciones Sprite recuperadas. |
| PlayerSpriteCollection::~PlayerSpriteCollection | Destrucción implícita C++ de ambos sprites. |
| TextWipe::~TextWipe | Marcada -; destrucción automática de sf::Text. |
| actionPress @00456370 | Traducida: frames=0, completa fade y coloca textos en destino sin resetear speed. |
| checkSkipState @0045ab50 | Traducida: ramas/actos según flags, THE_END nunca se omite por esta función. |
| draw @00456090 | Traducida: escena o nivel por capas, orden de entidades, partículas, textos y fade. |
| drawGoodEnding @004579c0 | Traducida: parallax día/noche, jugadores, caravana y 15 luciérnagas. |
| initializeCurrentState @00458b70 | Traducida: 14 estados, textos/posiciones/duración, ocho colas de entidades y parámetros. |
| initializePlayerSprites @004588b0 | Traducida: copia walk/jump de jugadores, reset/flip, colocación y stat SPD por API pública. |
| step @00455df0 | Traducida: fade/textos/desfile/ending y avance de estados con skips y fade de música. |
| stepGoodEnding | Traducida: movimiento/boarding, eliminación de jugadores y generación/animación de luciérnagas. |
| stepParade @004564e0 | Traducida: spawn, facing, entrada, texto, step/colisión, alfa/luz, retiro y actualización del nivel. |
| stepTextWipes @004563d0 | Traducida: dirección normalizada, velocidad proporcional limitada por aceleración y snap al destino. |
| lambda_58900e20c9f24808e6eafa1a8c9c1b5d @00456d10 | Integrada como remove: busca entrada por weak_ptr, desacopla todas las entidades, limpia vector y elimina esa entrada. |
| Level scalar deleting destructor | Marcada -; wrapper del compilador, ajeno al cuerpo de Credits. |
| ceilf | Wrapper CRT/MSVC; se usa std::ceil y no se duplica el runtime de C. |

Constantes: estados de texto240, thanks600, ending1200, desfile/THE_END65535;
view center320/180, texto16, boarding margen5/10, salto3.5, velocidad SPD*0.1+1.4,
caravana -0.2, cloud +0.1. Moon y=80 (distinto del menú). TextWipe acelera como
min(clamp(distance*0.0033333334140479565,0,1)*29.899999618530273+0.1,speed+0.1).
No se modifica el orden de operaciones, de entradas ni de llamadas RNG.

Los IDs se contrastaron con escrituras movb del ejecutable, no solo con las
etiquetas de Ghidra. Cada firefly consume dos canonical float (y,x) y dos int
(frame,timer): 60 sorteos al crear las15, ninguno en la animación posterior.
canonical @0041c980 redondea un MT unsigned a float y divide por2^32; no añade
rechazo. Sus límites globales son0 y1. drawGoodEnding dibuja walkSprite durante
boarding, aun cuando walking=false; se conserva el cuerpo compilado.

Se usan getters/setters públicos para Player/GameEngine y Level
(revealArea, drawParticles, setClientSide/setServerSide). Varias dependencias
siguen como stubs; la prueba no acredita generación real ni IA del desfile.
Spawn devuelve null si la dependencia sigue stub: se retorna sin desreferencia
ni eliminar la entrada, no se simula su aparición. Los helpers setTriggered,
setTriggeredFromMainMenu, mouseAction, resized y updateStrings no tienen
pseudocódigo independiente y permanecen pendientes. stepFadeEffect se recupera
del inline de step; isTriggered y creditsAreOver son lectores del campo.

Verificación en _local/menus/validation (aislada del Client.cpp en curso, como
explica RAMA_A_MAINMENU.md): build_recon_pc.ps1 -Run compiló, ejecutó y cerró con
exit code: 0. Credits_test pasó: 14 registros, parámetros de colas, textos,
fade/skips, acción, iluminación/retiro con entidades fixture, boarding, RNG60,
lifecycle y render. Se usan texturas sintéticas y fuente local; nada de ello
se añade como recurso a Git. La rama completa sigue pendiente del Client ajeno.
Logs _local/menus/Credits_build.log y Credits_test.log. Sin push ni prueba Vita.
