# Referencias públicas para el port de Vagante

Revisadas el 17 de octubre de 2026. Se evaluaron para encontrar código que
pudiera integrarse en el runtime actual (`vita/src`) y técnicas que ayuden a
completar ACT_ONE.

## Repositorios de Vagante

[`mtolly/vagante-extract`](https://github.com/mtolly/vagante-extract) extrae y
reempaca `data.vra`. Es la referencia pública más directa para el contenedor
del juego. El proyecto ya cuenta con un parser VRA propio y pruebas para
truncamiento, compresión, rutas inseguras y duplicados (`tools/vra.py`,
`tests/test_vra.py`), así que no hace falta incorporar un extractor paralelo.
La raíz pública consultada no muestra un archivo de licencia; se usa como
referencia de formato y no se copia su implementación.

No apareció en las búsquedas un repositorio público de decompilación o una
implementación completa del motor de Vagante que podamos integrar. Por ahora,
los ejecutables, PDB, listados de Ghidra y recursos privados siguen siendo la
fuente de verdad para recuperar comportamiento; esos datos permanecen fuera
del repositorio publicado.

## Referencias de ports para Vita

[`0xl0cal/repentogxm`](https://github.com/0xl0cal/repentogxm) muestra un flujo
de recompilación estática de un ejecutable Windows a C, seguido por VitaSDK y
vitaGL. Su utilidad inmediata es metodológica: fija la versión/hash de entrada,
separa código generado de recursos que aporta el usuario, documenta métricas
medidas y conserva fallbacks cuando una sustitución nativa no está probada.
No es una biblioteca de recompilación genérica para Vagante; su generador,
ABI, intérpretes y parches corresponden al ejecutable de Isaac. El repositorio
declara GPL-2.0, mientras VaganteVita está bajo MIT, así que no importamos
código sin una decisión de licencia y una revisión de compatibilidad.

[`Franckrst/D2Vita`](https://github.com/Franckrst/D2Vita) ejecuta el binario
original de Diablo II mediante un cargador PE, un runtime Win32 y un dynarec
x86→ARMv7 basado en Box86. Es una referencia útil para separar el runtime
genérico, los puentes del sistema operativo y la lógica específica del juego,
además de sus diagnósticos de arranque. No encaja como dependencia: VaganteVita
reconstruye lógica en C y no tiene ese ABI/runtime x86. D2Vita declara GPL-3.0.

[`rsn8887/SDLPoP-Vita`](https://github.com/rsn8887/SDLPoP-Vita) sirve como
ejemplo de empaquetado Vita y de SDL2/vita2d. El proyecto actual ya compila con
VitaSDK y tiene renderer, entrada y empaquetado propios; cambiar el renderer
por este proyecto no reduce el trabajo pendiente de ACT_ONE.

[`ggdrn/KH-DAYS-VITA`](https://github.com/ggdrn/KH-DAYS-VITA) separa la
decompilación upstream de los cambios Vita mediante una revisión fijada,
parches exportables y una capa que emula el hardware esperado. No es una base
de código compatible con Vagante, pero sí aporta un patrón que podemos usar en
la documentación de Ghidra: asociar cada función reconstruida con el hash del
ejecutable, la dirección y el listing fuente, y conservar las adaptaciones
Vita fuera de la descripción del comportamiento original.

[`sp00nznet/vitarecomp`](https://github.com/sp00nznet/vitarecomp) publica una
tubería de análisis, traducción, compilación y ejecución con fases nombradas,
fallos explícitos y límites del soporte por etapa. Su entrada son juegos
compilados para Vita, así que no puede recompilar Vagante de PC. El patrón
aplicable aquí es endurecer nuestras puertas de validación para que “paquete
preparado”, “replay generada”, “proceso Vita3K creado”, “sesión nativa abierta”
y “salida de ACT_ONE alcanzada” sean resultados distintos, nunca inferidos
unos de otros.

[`robin994/battleship-vita`](https://github.com/robin994/battleship-vita) pide
extraer los datos en escritorio y cargarlos desde almacenamiento en Vita; sus
cambios de plataforma también están marcados en el código portable. Es
coherente con el exportador ACT_ONE actual, que prepara paquetes de mundo y
atlas antes de instalar el VPK. No hace falta llevar un extractor pesado al
runtime de Vita.

## Decisión para ACT_ONE

No se importó código de terceros. Las partes aprovechables son los métodos de
validación y separación del trabajo, no sus motores específicos. Para ACT_ONE
se debe mantener trazabilidad de evidencia Ghidra por función y hash, y el
smoke Vita3K debe comprobar cada fase desde el proceso del emulador hasta el
log nativo y la salida real. La siguiente inversión de ingeniería sigue siendo
recuperar el comportamiento que falta en Vagante (IA y telaraña de Spider,
combate, interacciones y salida), y resolver el arranque de Vita3K con una
reproducción que genere log nativo.
