# Rama A — GameGui

Contrato PDB intacto. Inventario: `pseudo_index.py ... --unit GameGui`. Evidencia Ghidra y desensamblado en `_local/gui-db/`; se usaron field_writes.py, offset_name.py y call_args.py.

| Función indexada | Estado |
|---|---|
| GameGui::GameGui @005cd300 | traducida; cinco sprites, temporizadores, contadores y orígenes |
| GameGui::~GameGui | traducida mediante destructor C++ por miembros |
| GameGui::drawGameVersion @005cec10 | traducida; versión/parche, sufijo Online e idioma de Config |
| GameGui::resized @005cd6d0 | traducida; desplazamiento (tamaño de vista −640×360)/2 |
| GameGui::drawControllerUpdates | traducida; selección de icono por temporizador y seno/raíz de alfa |
| GameGui::drawExperienceBar | traducida; interpolación, brillo, decaimiento y sorteo del efecto |
| GameGui::drawFpsCounter | traducida; dos textos y umbrales41/51 |
| GameGui::drawGoldDisplay | traducida; banner, moneda y números por atlas |
| GameGui::drawTime | traducida; ticks a minutos:segundos, centrado y timerPosition |
| GameGui::drawTransitionCircle | traducida; tira de62 vértices, radio exterior diagonal, interior radio/256 |
| GameGui::drawNetGraph | excluida por instrucción: diagnóstico de red; cuerpo vacío con comentario |
| lambda_ae0ede928685dab13a0350b39c951f07::operator() | traducida como drawNumber en drawGoldDisplay |

Constantes: sprites font16×16, banner64×16 y moneda16×16, iconos completos; timerPosition=-10. Versión tamaño16/escala.5 en(3,0), RGBA(255,255,255,20); idioma(150,150,255,22). Moneda en(width−61,7), banner(width−66,10), números a width−14 con paso10/y6 o22 y banco de atlas+13. Controles: x=width*(i/4+1/8), y=height−42−25*sqrt(alpha). Experiencia: delta*.5, brillo3 con decaimiento*.1*delta, efecto(counter−delta*.03)*.89; un canonical float MT por efecto visible, sin rechazo adicional. Círculo: 62 vértices, ángulo.20943953096866608, radio*.00390625*diagonal.

Ghidra desplaza campos y confunde sf::View::getSize @00814110 con RenderTarget::getDefaultView; se verificó en asm. El float de drawExperienceBar está en XMM2 y es delta, el int en la pila es experiencia; el bool retenido no se usa. El Player de drawTransitionCircle tampoco se usa en el cuerpo compilado. Se mantienen las firmas. getLanguageString recibe config.language (+0x34), no el cookie de Ghidra.

No se redefinió ningún accessor de inline_accessors.cpp ni se modificaron Client, NetMenu, GuiCursor o ConfigGui. reset, step, drawPausedOverlay, drawDpsCounter, drawHotkeys, drawMinimap y GoldPiece::GoldPiece no figuran con pseudocódigo en esta unidad: sus stubs quedan explícitamente pendientes, sin inventar comportamiento.

Verificación: `pwsh tools/build_recon_pc.ps1` terminó en OK. `pwsh tools/run_recon_tests.ps1 -Unit GameGui` pasó: constructor sin RNG, resized con vista distinta, etiquetas realmente dibujadas, ticks/FPS, posiciones y frames del oro, iconos, contadores y secuencia RNG, y máscara de transición observada por píxeles. Font local/texturas sintéticas; ningún recurso comercial añadido. Logs `_local/gui-db/GameGui_build.log` y `GameGui_test.log`. Pruebas host, sin validación de gameplay Vita. Sin push.
