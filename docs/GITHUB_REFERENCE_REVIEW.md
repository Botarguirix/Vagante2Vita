# Revisión de referencias de GitHub para VaganteVita

Revisión: 17 de octubre de 2026. Se contrastaron referencias públicas con la arquitectura y las herramientas presentes en este repositorio. La existencia de una técnica en otro port no demuestra que sea compatible con el ejecutable de Vagante.

## Hallazgos aplicables

### REPENTOGXM: recompilación estática, pero otro punto de partida

[0xl0cal/repentogxm](https://github.com/0xl0cal/repentogxm) traduce funciones de un ejecutable x86 de Isaac a C, añade un runtime y compila para ARM/Vita. Conserva fallbacks traducidos cuando una optimización nativa no satisface sus precondiciones. Es un precedente útil para automatizar análisis, traducción por función, pruebas y optimización selectiva.

Vagante parte de un EXE Win32 x86 con PDB y dependencias SFML/OpenGL/OpenAL/Steamworks. El port actual es una reconstrucción nativa por sistemas, no un traductor completo. La arquitectura de REPENTOGXM no puede copiarse tal cual: antes harían falta recuperación de ABI/llamadas x86, CRT y dependencias, y una capa de compatibilidad sustancial. La guía de build del repositorio también exige un VitaSDK soft-float específico, dato que conviene verificar contra el toolchain configurado localmente antes de cambiar ABI.

Prácticas que sí adoptamos como criterios: código y recursos del juego permanecen locales/provistos por el usuario; registrar hashes y versión de la entrada; medir FPS, memoria y stalls por escena; etiquetar optimizaciones como medidas y conservar un camino de referencia cuando una transformación no esté demostrada. No copiamos código del port sin revisar su licencia y compatibilidad.

### Ghidra por lotes: export reproducible y con caché

[totekuh/ghidra-decomp](https://github.com/totekuh/ghidra-decomp) propone exportar pseudocódigo, ensamblador, xrefs, tipos, callgraph y cadenas a archivos navegables, conservar análisis cacheado con fingerprint de binario/configuración y reanudar exportaciones por hash.

Ya tenemos un análisis Ghidra local grande, pseudocódigo exportado y scripts de auditoría que exigen referencias a EXE/PDB. La mejora útil sería añadir un manifiesto versionado de procedencia y un índice de xrefs/callgraph para las funciones priorizadas. La herramienta externa requiere Ghidra 12+ y PyGhidra 3+, así que no se incorpora automáticamente: primero hay que comprobar versiones locales y que la salida sea comparable con el análisis existente, sin duplicar decenas de miles de funciones.

### vitaGL y puertos de referencia: validar la ruta de render primero

La lista de proyectos de [Rinnegatamante/vitaGL](https://github.com/Rinnegatamante/vitaGL) confirma puertos 2D, motores y frontends SDL2 que usan este backend. Sin embargo, Vagante actualmente presenta un framebuffer software lógico 320×180 y escala filas a pantalla; no enlaza vitaGL ni SDL. Migrar ahora aumentaría el alcance sin resolver el fallo actual de arranque de Vita3K.

El informe de [goldenballoon-vita-port](https://github.com/zm2283145/goldenballoon-vita-port/blob/main/PORTING_STATUS.md) contiene un caso de fallo específico: su `vglInitExtended` no presentó imagen al faltar el presupuesto de memoria contigua `-pm 0x2000000` en `vita-make-fself`. Eso no aplica al renderer software actual. Lo registramos como comprobación obligatoria si se adopta vitaGL, no como cambio al empaquetador vigente.

## Decisión de trabajo

1. Mantener la reconstrucción por sistemas mientras el bucle software siga siendo la ruta más pequeña que puede validarse.
2. Resolver la evidencia de arranque Vita3K (diálogo de error, log vacío y proceso que muere antes de la sesión nativa) antes de atribuirle un defecto al gameplay.
3. Cuando el renderer cambie a vitaGL, hacer un prototipo aislado de inicialización/presentación y medir memoria; revisar flags de SELF, dialecto shader, caché y fallbacks contra versiones fijadas.
4. Mejorar la trazabilidad Ghidra con datos locales y scripts reproducibles, sin incorporar EXE, PDB, recursos, dumps ni pseudocódigo generado al repositorio público.

## Fuentes

- [repentogxm README](https://github.com/0xl0cal/repentogxm/)
- [vitaGL README y proyectos](https://github.com/Rinnegatamante/vitaGL)
- [ghidra-decomp](https://github.com/totekuh/ghidra-decomp)
- [goldenballoon Vita porting status](https://github.com/zm2283145/goldenballoon-vita-port/blob/main/PORTING_STATUS.md)
- [VitaSDK samples](https://github.com/vitasdk/samples)
- [decomp.wiki / Decompedia](https://decomp.wiki/)

## Decompedia: separar las metas y medir evidencia

[Decompedia](https://decomp.wiki/) cataloga recursos para decompilación de
videojuegos: reportes de progreso decomp.dev, comparación colaborativa de
funciones en decomp.me y un servidor Ghidra compartido
([página principal](https://decomp.wiki/)). Su valor para este proyecto es de
orientación y descubrimiento de herramientas; no ofrece una vía automática que
convierta pseudocódigo de Ghidra en el juego original recompilado.

La propia distinción de la wiki entre *matching decompilation* y otros tipos
de proyecto cambia cómo debemos describir VaganteVita: matching busca código
fuente que reproduzca el binario original con su toolchain, mientras que aquí
estamos reconstruyendo sistemas para una implementación ARM nativa. La página
de progreso de decomp.dev es útil para entender la métrica de coincidencia,
pero un porcentaje de funciones matching no mediría el avance de este port.
Tampoco el uso de un servidor compartido de Ghidra sustituye nuestro proyecto
local: el análisis contiene un ejecutable y PDB propietarios, así que mantener
esa base de datos local evita publicar o cargar esos materiales.

Aplicación al flujo actual:

1. Registrar cada hallazgo de ingeniería inversa con dirección, símbolo
   inferido, versión/hash de entrada cuando esté disponible, evidencia
   (listing, pseudocódigo, referencias cruzadas) y nivel de confianza.
2. Mantener aparte el avance del port: sistemas implementados, diferencias
   conocidas, pruebas host y pruebas de Vita3K/hardware. Un resultado de host
   no se presenta como validación en consola.
3. Usar decomp.me solo si aparece una necesidad real de igualar una función
   concreta y se puede compartir un snippet limpio y redistribuible. No subir
   pseudocódigo, binarios ni datos extraídos del juego.
4. Tratar decomp.dev como un directorio/progreso de proyectos, no como una
   métrica para declarar completa la reimplementación.

Esto coincide con las limitaciones ya anotadas en las iteraciones 103 y 110:
la evidencia Ghidra sirve para recuperar comportamiento puntual, pero una
función analizada no prueba fidelidad visual, de gameplay ni paridad completa.
La lectura de Decompedia aporta un marco de clasificación; las decisiones de
compatibilidad siguen necesitando fuentes primarias del código, toolchain y
pruebas de este port.
