# Iteración 032: transición provisional entre pisos de ACT_ONE

La aplicación ahora intenta cargar el siguiente paquete al tocar la salida de
ACT_ONE. El cargador prepara mundo, entidades, enemigos, terreno y jugador en
un objeto temporal; si carga correctamente, cambia el estado activo y conserva
la salud restante. El piso 0 usa los archivos existentes; los pisos 1 y 2 usan
`floors/floor-01.*` y `floors/floor-02.*`. Al terminar el piso 2, el juego
regresa al selector y registra que falta implementar el jefe/transición final.

El paquete privado de prueba contiene tres pisos para la semilla 12345. El
generador exportó pisos provisionales de 80×40: el piso 1 contiene 29 entidades
y 8 enemigos provisionales; el piso 2 contiene 45 entidades y 10 enemigos
provisionales. La ruta auditada es una conexión de tiles y una comprobación
geométrica del punto de aparición; todavía no prueba que la física nativa
permita completar el recorrido. No hay paridad exacta con el generador original
y la campaña se marca explícitamente incompleta.

La compilación VitaSDK produjo
`_local/build-act-one-032/Vagante2Vita-iteration032.vpk` (SHA-256
`95D42FBA3E314D19937CC9C10695AEDF8E5542CF9AF18337B923AE2832212A4D`). El VPK
no incluye datos comerciales. La validación del bundle de campaña encontró 65
archivos válidos, y la suite Python pasó 114/114 pruebas, incluidas las nuevas
aserciones contra logs de una iteración anterior.

Se probaron tres lanzamientos del smoke: Vita3K 0.2.1.4183 con SDL/OpenGL (45
segundos), un clon 0.2.1.4170 con SDL/Vulkan (45 segundos) y 0.2.1.4183 con
Qt/Vulkan (120 segundos). Ninguno creó log nativo; el último tampoco abrió una
ventana. `--version` quedó sin salida durante cinco segundos en las dos
versiones. El log existente de `G:\Vita3k` corresponde a la versión de juego
00.26 y no valida 00.32. El proceso Vita3K del otro proyecto permaneció intacto.
Por tanto, el arranque, la carga del piso 0 y el cambio al piso 1 siguen sin
validación de ejecución.

La auditoría del historial encontró además que una corrida etiquetada como
iteración 026 cargó en Vita3K la versión 00.25. El harness ahora exige que la
versión del `BOOT` nativo y la versión del título informada por Vita3K coincidan
con la iteración pedida; hay una prueba de regresión para rechazar versiones
anteriores. Esta mejora evita atribuir una ejecución de otra compilación al
VPK actual, pero no reemplaza la ejecución pendiente de 00.32.

Para considerar completo el primer mundo aún hacen falta, como mínimo:

- validar que el jugador puede recorrer físicamente cada piso y tocar cada
  salida, además de cargar el piso siguiente sin perder/romper el estado;
- completar los sistemas de enemigos (IA, ataques, daño, reacción y muerte) y
  comprobar interacciones de cofres, trampas, tiendas y objetos;
- reconstruir la generación ACT_ONE y su RNG con paridad reproducible; los
  pisos 1–2 actuales son sustitutos deterministas, no la generación original;
- implementar y probar el jefe y la transición posterior al piso 2;
- validar guardado, memoria, rendimiento y controles en Vita física.

La transición añadida es código compilado, pero todavía no verificado en
ejecución por la limitación de Vita3K. No se afirma que ACT_ONE esté completo.
