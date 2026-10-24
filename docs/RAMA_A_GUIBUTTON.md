# GuiButton — rama A

2026-10-24. Índice --unit consultado; cabeceras intactas.

| Función del índice | Estado |
|---|---|
| GuiButton::GuiButton @005e4b70 | Traducida: base, flags, nombre/tooltip y sprites 8x8 y 8x16 con origen cero. |
| GuiButton::action @005e4dc0 | Traducida: armado al pulsar, evento al soltar, ignora disabled. |
| GuiButton::mouseAction @005e4df0 | Traducida: ignora hidden/disabled, evento al soltar y rightClick=!left. |
| GuiButton::step @005e4e30 | Traducida: hidden cancela pressed; hover incrementa temporizador. |
| GuiButton::afterStep @005e4e60 | Traducida: completa una pulsación al perder hover o liberar ambos botones. |
| GuiButton::draw @005e4e90 | Traducida: borde sin atlas, nine-slice, pulsado +9, BlendAdd, alfa y texto SFML UTF-8. |
| GuiButton::pollAction @005e57f0 | Traducida: sonido menu/menu_use_button.wav sin posición/loop, consume evento. |
| scalar deleting destructor | Marcada -; generación automática. Destructor C++ default conserva destrucción de sprites. |

Accessors simples de campos incluidos: setUseBackground, updateName, wasRightClick, setToggleState, getToggleState, getName. setThick y activate no aparecen en el índice y permanecen stubs; no se inventa semántica. isATab y tabSprite no se usan en el draw compilado recuperado.

Ghidra confunde los campos heredados con _padding_: el desensamblado fija hidden +0x2c, pressed +0x2d, disabled +0x2e y hover +4. La firma lógica del constructor se conserva exactamente de la cabecera; el binario optimizado reordena argumentos físicos. Constantes inline: borde 1/2, tiles 8, estado pulsado +9, overlay alfa=alphaModifier*50, texto tamaño16/escala0.5. HOVER_TOOLTIP_DELAY no tiene valor recuperable ni uso en estos cuerpos; se mantiene el TODO del generador.

Verificación: build_recon_pc.ps1 -Run compiló y cerró con exit code: 0. Prueba GuiButton: pulsar/soltar, dos botones, hidden/disabled/hover/afterStep y render de borde/atlas pasó. Los fixtures proporcionan Config y atlas procesado con padding, igual que ResourceManager; los intentos iniciales incompletos del fixture se corrigieron. Logs: _local/gui/GuiButton_build.log y GuiButton_test.log. No se declara validado en Vita.
