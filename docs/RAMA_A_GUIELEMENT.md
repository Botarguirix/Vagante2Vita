# GuiElement — rama A

2026-10-24. Cabeceras intactas. Índice --unit consultado.

| Función | Estado |
|---|---|
| GuiElement::GuiElement @005e64f0 | Traducida: todos los campos, cursorOffset=(0,11), alpha=1. |
| GuiElement::~GuiElement @005e65b0 | Traducida mediante destructor STL automático. |
| GuiElement::mouseAction @005e65c0 | Traducida: cuerpo vacío del original. |
| GuiElement::setOffset @005e65d0 | Traducida: asigna ambos componentes. |
| action, step, afterStep | Marcadas - en índice; cuerpos vacíos de la base. |
| scalar deleting destructor | Marcado -; lo genera el compilador. |

Accessors sin pseudocódigo independiente implementados como lecturas/escrituras del campo PDB para permitir usar la base en las siguientes unidades. contains se recupera de su expansión en GuiWindow::mouseAction @005ec3e0: !hidden y límites [x,x+w), [y,y+h), sin offset. No se añaden clamps ni otros efectos a setters.

Constantes recuperadas de inicializadores dinámicos: hoverColor=(90,255,255,255) @00402ac0; disabledColor=(100,100,100,255) @00402ae0. Los ceros en statics.cpp eran la imagen previa al inicializador. Se definen en el .cpp y el generador omite los duplicados.

Verificación: pwsh tools/build_recon_pc.ps1 -Run terminó con exit code: 0; run_recon_tests.ps1 -Unit GuiElement pasó (campos, límites, hidden, offset, accesores, virtuales vacíos, colores). Logs _local/gui/GuiElement_build.log y GuiElement_test.log. Sin push ni validación de gameplay Vita.
