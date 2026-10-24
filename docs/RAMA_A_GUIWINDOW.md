# GuiWindow — rama A

2026-10-24. --unit GuiWindow consultado; cabeceras intactas.

| Función del índice | Estado |
|---|---|
| GuiWindow::mouseAction @005ec3e0 | Traducida: hit-test de hijos visibles con límites semiabiertos, orden de inserción, mismos argumentos. |
| GuiWindow::step @005ec460 | Traducida: propaga a todos los hijos. |
| GuiWindow::afterStep @005ec4b0 | Traducida: propaga ambos botones a todos los hijos. |
| GuiWindow::draw @005ec500 | Traducida: hidden suprime draw; fondo negro opcional con borde1, luego hijos en orden. |
| GuiWindow::setOffset @005ec640 | Traducida: solo propaga a hijos, sin asignar offset propio. |
| scalar deleting destructor | Marcada -; no se copia el wrapper. Se recuperó su lógica de destrucción desde asm @005ec330: delete de cada hijo, vector y base automáticos. |

Constructor y add están expandidos en EditorState::EditorState @00583ed0: base WINDOW, elementos vacíos, hasBg=true y push_back sin cambiar coordenadas ni offset. Se reconstruyen con la firma PDB (floats convertidos a int). setHasBg asigna el flag. setAlpha no aparece en índice ni tiene cuerpo recuperado: permanece stub; no se inventa propagación.

El draw usa x/y y w/h sin offset ni alphaModifier. Mouse/step/afterStep no comprueban hidden de la ventana; mouse solo filtra hidden de cada hijo, sin filtrar disabled desde la base: cada control procesa su estado. Los bucles vuelven a consultar size tras cada llamada y admiten que un hijo añada otro durante step, como el original. La ventana es propietaria de sus punteros; no se añaden checks de nulidad a dispatch que no existen en el original.

Verificación: pwsh tools/build_recon_pc.ps1 -Run compiló y cerró con exit code: 0. run_recon_tests.ps1 -Unit GuiWindow pasó: dispatch y orden, límites, estados, render/fondo, offset anidado, inserción durante step y eliminación de hijos. Integra GuiButton/GuiTextBox con fuente SFML local: consumo de acción y sonido silenciado, variantes de texto, edición y liberación del buffer al destruir la ventana. Logs _local/gui/GuiWindow_build.log y GuiWindow_test.log. Sin push ni validación Vita.
