# GuiTextBox — rama A

2026-10-24. --unit GuiTextBox consultado; cabeceras intactas.

| Función del índice | Estado |
|---|---|
| GuiTextBox::GuiTextBox @005e9bf0 | Traducida: base TEXTBOX, float→int, altura fija10, flags/cadenas/buffer. |
| GuiTextBox::~GuiTextBox @005e9cf0 | Traducida: libera captura de Input solo si coincide buffer; destrucción STL automática. |
| GuiTextBox::startEnteringText @005e9d90 | Traducida: foco, buffer, cursor al final y allowNewLines mediante Input::setEnteringText. |
| GuiTextBox::stopEnteringText @005e9e20 | Traducida: Input::stopEnteringTextIfBufferEqual. |
| GuiTextBox::mouseAction @005e9e60 | Traducida: hidden/disabled, pulsación izquierda y logs VLOG. |
| GuiTextBox::step @005e9fd0 | Traducida: incrementa frame; fin de entrada activa evento y quita foco. |
| GuiTextBox::draw @005ea080 | Traducida: caja, fuente Unicode, label lateral/superior, cadena ligada, cursor/selección multilínea. |
| GuiTextBox::bindString @005ea7f0 | Traducida: bound, puntero y diagnóstico VLOG. |
| scalar deleting destructor | Marcada -; generación automática. |

Los campos privados de Input expandidos por Ghidra se usan mediante sus métodos públicos, sin editar cabeceras. stopEnteringText no borra focus: el siguiente step observa el fin de entrada, como el original. Destruir otro cuadro no roba el buffer activo. bgLabel se almacena pero no participa en el draw recuperado. Los setters de labels, pollAction y getString son wrappers simples sin cuerpo independiente en índice, reconstruidos para permitir el uso público del control.

Constantes: altura10 (push 0xa antes de constructor base), borde0.5, texto16/escala0.5, margen7, texto x+3, label superior y-11, selección alto9 y líneas8, alfa del cursor=sin(frame*0.05)*50+110. El cuarto float del constructor queda sin usar en el cuerpo compilado; se conserva la firma PDB. draw no aplica offset ni alphaModifier; no se cambia silenciosamente. Los índices del cursor se pasan sin normalizar en la selección de una sola línea, igual al original.

Adaptación para entrada inválida: bindString(nullptr) no desreferencia para log y startEnteringText lo pasa a Input sin medirlo; draw omite un buffer nulo. El original podía fallar con ese binding. No cambia los bindings válidos. Sin Windows/Steam ni RNG.

Verificación: pwsh tools/build_recon_pc.ps1 -Run compiló y ejecutó, cierre normal exit code: 0. Prueba GuiTextBox aprobada: edición real por Input, foco/eventos, ownership y destructor, render con fuente SFML local, selección de una/múltiples líneas y sentido inverso, flags y buffer nulo. La fuente de prueba se lee del sistema, no se copia a Git. Logs _local/gui/GuiTextBox_build.log y GuiTextBox_test.log. Sin push ni validación Vita.
