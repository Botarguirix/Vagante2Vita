# Sprite — tarea 5

2026-10-24, rama `rama-a`. Sin push. Cabeceras PDB sin cambios.

Se tradujeron todos los cuerpos con pseudocódigo del índice de la unidad Sprite,
incluidas las cuatro sobrecargas del constructor y la asignación. Se implementan
animación, atlas, triángulos SFML, carga diferida, máscaras y registro de dueño.
El lookup de paletas conserva la llamada a ResourceManager; su implementación
`getTextureAltPalette` sigue pendiente en esa otra unidad.

## Verificación

- `pwsh tools/build_recon_pc.ps1 -Run`: compilación y enlace sin errores,
  ventana nativa ejecutada y cerrada normalmente, **exit code: 0**.
  Evidencia: `_local/sprite-vfont/sprite-build-run-2.log`. `recon.log` alcanzó
  55 funciones pendientes; ninguna de Sprite.
- `pwsh tools/run_recon_tests.ps1 -Unit Sprite`: pasó. Comprueba el límite
  estricto del timer, reinicio, reversa, bucle/no bucle, delay, freeze, posiciones
  con offset y flip, escalas, colores/alfa, copia/movimiento y registro/eliminación
  del dueño. Cuatro frames de un atlas sintético se renderizan y comparan por
  píxel mediante SFML; salida `_local/recon-tests/Sprite/work/sprite-fixture.png`.
- La entidad usada por el fixture implementa sus métodos abstractos como no-ops;
  la prueba comprueba el registro de sprites, no el comportamiento de Entity.
- No se compila un VPK ni se valida gameplay en Vita en esta tarea.

## Evidencia, constantes y dudas

- `Sprite::step` @007b9720: avanza cuando `timer > speed`. En frame 0, el delay
  se conserva mientras `timer < resetDelay + speed`; freeze impide también
  actualizar el rectángulo por desplazamiento/escala.
- `setFrame` @007b9c20 usa tiles con borde `(w+2, h+2)` más un píxel cuando
  `tileProcessed` está activo. `step` y `reset` usan el grid sin padding. Se
  conserva la diferencia del original, cubierta por la prueba.
- `lazyLoadTexture` @007b9f00 calcula `numFrames == -1` antes de corregir el grid
  de un atlas `@` procesado. Se conserva ese orden, incluso cuando produce un
  número de frames distinto de `rows * cols` después de corregir el grid.
- Origen vacío `(0.5,0.5)`; origen normal `(w/2,h/2)`; cuatro vértices en orden
  `(0,0),(0,h),(w,0),(w,h)`; `sf::TriangleStrip`. Flip afecta el signo X y conserva Y.
- Constructor de copia @007b8ac0: nombres de paleta inicialmente vacíos, timer
  puesto a cero por `setFrame(frame)`; el dueño puede reaplicar su paleta.
  Constructor @007b8f70: sus dos bools no se usan en el cuerpo compilado.
- El tipo de la función @007b9250 se verificó directamente en el PDB: es
  `Sprite &operator=(Sprite &&)`, no la sobrecarga lvalue. La operación mueve el
  registro del dueño, pero copia campos/cadenas. El original no elimina primero
  el registro del destino en un dueño anterior; no se inventa aquí esa limpieza.
- La asignación lvalue no tiene cuerpo en el índice: se reconstruye componiendo
  copia y movimiento, conservando el registro del origen. Es una inferencia.
- El PDB generado no declara la amistad Sprite/SpriteEntityInterface. Se usa un
  pequeño puente estándar C++ de punteros a métodos privados, sin editar la
  cabecera ni cambiar layout/access labels. `addSprite`, `setPalette` y el erase
  inline de `removeSprite` se incluyen como dependencias en `Sprite.cpp`.
- `tools/pdb_stubs.py` reconocía `operator=` como el identificador `operator`
  seguido por un inicializador. La prioridad de las alternativas se corrigió
  para operadores: la regeneración ya retira ambas sobrecargas del stub. La
  compilación/enlace y la prueba de asignación verifican esta corrección.
- No se introducen llamadas al RNG. Los helpers sin pseudocódigo que implican
  cálculo temporal o aleatoriedad no se inventan.

## Inventario del índice

| Función | Estado |
|---|---|
| `Sprite::Sprite()` | Traducida, @007b86d0. |
| `Sprite::Sprite(owner,name,w,h,frames,speed,dx,dy,loop,mask)` | Traducida, @007b8870. |
| `Sprite::Sprite(const Sprite&)` | Traducida, @007b8ac0. |
| `Sprite::Sprite(const Sprite&,bool,bool)` | Traducida, @007b8f70. |
| `Sprite::~Sprite` | Traducida, @007b9170. |
| `Sprite::operator=(Sprite&&)` | Traducida, @007b9250. |
| `Sprite::draw` | Traducida. |
| `Sprite::getFlipped` | Traducida. |
| `Sprite::getFrame` | Traducida. |
| `Sprite::getPosition` | Traducida. |
| `Sprite::getTime` | Traducida. |
| `Sprite::isFinished` | Traducida. |
| `Sprite::lazyLoadTexture` | Traducida. |
| `Sprite::reset` | Traducida. |
| `Sprite::setColorComponents` | Traducida. |
| `Sprite::setFlipped` | Traducida, argumentos/composición contrastados con desensamblado. |
| `Sprite::setFrame` | Traducida. |
| `Sprite::setFrozen` | Traducida. |
| `Sprite::setPosition` | Traducida, ambas sobrecargas juntas. |
| `Sprite::setSpeed` | Traducida. |
| `Sprite::setTexture` | Traducida. |
| `Sprite::setTextureRect` | Traducida. |
| `Sprite::step` | Traducida. |
| `Sprite::scalar deleting destructor` | Marcada `-`; generado por el compilador, no se redefine. |

Los accesores de campos, colores, alpha, offset, palette, transforms y helpers
de vértices expandidos en estos cuerpos también están implementados. La cabecera
declara además los siguientes nombres sin cuerpo independiente ni entrada en el
índice de pseudocódigo: `drawDebug`, `setRandomFrame`, `percentComplete`,
`setPercentComplete`, `justStartedFrame`, `getNextTicksFrame`, `getTotalFrames`.
Se conservan sus stubs; están fuera del conjunto de funciones con pseudocódigo
de esta tarea y requieren recuperar sus expansiones antes de traducirlos.
