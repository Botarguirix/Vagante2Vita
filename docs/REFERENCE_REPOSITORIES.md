# Referencias públicas para el port de Vagante

Revisadas el 17 de octubre de 2026. Se evaluaron para encontrar código que
pudiera integrarse en el runtime actual (`vita/src`) y técnicas que ayuden a
completar ACT_ONE.

[`Decomp.wiki / Decompedia`](https://decomp.wiki/) cataloga proyectos de
decompilación de videojuegos, herramientas y recursos por plataforma. No debe
confundirse con [`decompilation.wiki`](https://decompilation.wiki/), la wiki
técnica general sobre análisis binario que también revisamos. Ninguna contiene
una base de código o decompilación de Vagante que podamos integrar. Decompedia
describe matching decompilation como reconstruir código que, con el compilador
y entorno originales, produzca el mismo binario. Este proyecto busca recuperar
comportamiento y recrearlo para ARM/Vita; compilar un VPK no demuestra matching
ni el pseudocódigo de Ghidra es fuente recompilable. Seguiremos registrando
función/dirección y evidencia nativa en Ghidra, y validando por separado el
comportamiento equivalente en Vita.

La comprobación más reciente queda en [ITERATION118.md](ITERATION118.md):
Decompedia orienta sobre matching y recursos de la comunidad, pero no aporta
una ruta automática para este port; no subimos análisis propietarios a
servicios compartidos.

La revisión directa de la portada y sus páginas de recursos confirma que
Decompedia agrupa proyectos por plataforma, compiladores, herramientas y
recursos comunitarios. Sus enlaces centrales incluyen decomp.dev (seguimiento
de progreso), decomp.me (práctica colaborativa de matching) y un servidor Ghidra
compartido. No es un servicio de análisis automático al que debamos enviar el
EXE. Las páginas de plataformas explican además que las herramientas de
matching dependen de la arquitectura, el formato de objeto y la versión del
compilador; por ejemplo, varias utilidades citadas para PS1/N64 esperan MIPS o
ELF y no corresponden al PE x86 de Vagante.

Aplicación para nuestro flujo: usar la wiki como índice de conceptos y
referencias, y trasladar sólo prácticas independientes de plataforma:
registrar binario/PDB coincidentes, separar hechos observados de hipótesis,
documentar incertidumbres y verificar cada comportamiento en pruebas propias.
No hay un repositorio o herramienta de la portada que sustituya Ghidra 12.1.4,
el exportador local o la reconstrucción funcional ya implementada. El servidor
Ghidra/decomp.me compartidos no son apropiados para nuestros artefactos
privados. Esta revisión es de orientación técnica, no evidencia de paridad ni
de una recompilación exacta.

## Recursos de decompilación

[`Decompedia`](https://github.com/decompals/decompedia) y su sitio
[`decomp.wiki`](https://decomp.wiki/) describen principalmente la decompilación
matching: reconstruir C/C++ que, con el compilador original y condiciones muy
precisas, genere el mismo binario. También indexa proyectos, herramientas y
recursos de distintas plataformas; no es un decompilador automático de Vagante
ni contiene una reconstrucción pública del juego. Sus notas recalcan que el
compilador/versión y la plataforma alteran los patrones generados. Las páginas
por consola incluyen herramientas de comparación de assembly como asm-differ,
m2c y permuters, pero varias asumen formatos/arquitecturas específicos (por
ejemplo, ELF y MIPS) y no son plug-and-play para el ejecutable Win32 de Vagante.

Aplicación concreta al proyecto: fijar y registrar hash, versión del EXE y
compatibilidad PDB antes de atribuir evidencia; para cada comportamiento,
conservar dirección, función, pseudocódigo/listing y datos usados; contrastar
las ramas relevantes con instrucciones x86, llamadas, RTTI/PDB y constantes; y
separar claramente el comportamiento observado en Ghidra de la aproximación
del runtime Vita. El proyecto hace reconstrucción funcional para una plataforma
nueva, no matching del ejecutable x86 original, por lo que perseguir igualdad
de bytes no sería una métrica útil. Decompedia también enlaza decomp.me y un
servidor Ghidra compartido; no se usarán para subir ejecutables, PDB, recursos
privados ni pseudocódigo de la copia del usuario. Véanse [Decompedia: inicio](https://decomp.wiki/)
y [Decompedia: PS1 y sus herramientas](https://decomp.wiki/en/platforms/playstation)
como ejemplos, no como guías directas para Windows x86.

La revisión de las páginas de recursos, PS1 y del proyecto
[`decomp.me`](https://github.com/decompme/decomp.me) deja estas decisiones:

- No usar `asm-differ`, m2c ni permuters como criterio de aceptación para el
  port Vita: son herramientas de matching y algunas presuponen compilador,
  objeto y arquitectura concretos que no corresponden al build ARM nuevo.
- Mantener el ejecutable y datos comerciales de entrada en `_local`; no subir
  a servicios compartidos (incluidos decomp.me o un servidor Ghidra) binarios,
  PDB, pseudocódigo ni recursos extraídos de la copia del usuario.
- Mejorar la trazabilidad local de cada afirmación de comportamiento con
  hash/versión del ejecutable, dirección y nombre Ghidra, xrefs relevantes,
  constante o campo observado, grado de confianza y prueba de runtime que lo
  cubre. Una interpretación todavía ambigua debe quedar como hipótesis y no
  como regla nativa confirmada.

Se revisó también [`totekuh/ghidra-decomp`](https://github.com/totekuh/ghidra-decomp),
un exportador masivo que genera un árbol buscable de funciones Ghidra, reutiliza
el proyecto analizado y puede reanudar exportaciones. Requiere Ghidra 12 y
PyGhidra 3; aún no se ha comprobado contra nuestra instalación, scripts ni
proyecto actual. No se añade como dependencia ni se ejecuta sobre binarios
privados hasta verificar compatibilidad y salida reproducible. El enfoque de
exportar por función y guardar artefactos versionados coincide con nuestros
scripts locales; podemos adoptar primero hashes y un manifiesto de evidencia
sin reemplazar el flujo que ya funciona.

## Hallazgos de las wikis y aplicación al análisis de Vagante

La wiki técnica de [`decompilation.wiki`](https://decompilation.wiki/) organiza
el proceso de análisis en recuperación de flujo de control, lifting a una
representación intermedia, identificación de funciones, resolución de saltos,
recuperación de tipos y recuperación de símbolos. Para nuestro trabajo, esto
se traduce en verificar cada función y sus ramas en el listing x86 de Ghidra,
apoyar las interpretaciones de campos en offsets/usos/RTTI/PDB, y mantener
direcciones y xrefs como procedencia. No hace falta añadir otra IR/decompilador
al port ahora: Ghidra ya proporciona el desensamblado y P-code para inspección.

Aplicado a la rama Thief de `addRandomEnemies`, un gate RNG conocido no basta
para afirmar que el enemigo se genera: todavía hay que comprobar la asociación
con la entrada, el orden de candidatos, filtros, decremento de presupuesto y
draw de coordenadas. Esta es una aplicación directa del análisis de CFG y
límites de función descrito por la wiki, y coincide con lo que aún marca como
pendiente [ITERATION125.md](ITERATION125.md).

La wiki explica que la evaluación total de una decompilación no tiene una
métrica universal aceptada. Por eso el ledger distingue evidencia estática,
pruebas host, empaquetado VPK, sesión nativa de Vita3K y recorrido real del
juego; contar funciones nombradas, líneas o VPKs no es un porcentaje de
paridad. Las pruebas deben comparar cada regla con evidencia original cuando
exista, y etiquetar como provisional lo que siga sin ground truth.

Fuentes metodológicas: [visión general de CFG](https://decompilation.wiki/fundamentals/cfg-recovery/overview/),
[program lifting](https://decompilation.wiki/fundamentals/cfg-recovery/lifting/),
[identificación de funciones](https://decompilation.wiki/fundamentals/cfg-recovery/function-recovery/),
[recuperación de tipos](https://decompilation.wiki/fundamentals/type-recovery/overview/),
[recuperación de símbolos](https://decompilation.wiki/applied-research/symbol-recovery/),
[evaluación](https://decompilation.wiki/fundamentals/evaluation/) y
[reconstrucción de programas](https://decompilation.wiki/applications/program-reconstruction/).
Estas páginas orientan el método; no contienen evidencia específica del código
de Vagante. Decomp.wiki/Decompedia es útil para descubrir proyectos y recursos,
pero las técnicas de matching listadas para consolas y compiladores concretos
no son plug-and-play para el ejecutable Windows x86 ni sustituyen la
reimplementación funcional para ARM/Vita.

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
