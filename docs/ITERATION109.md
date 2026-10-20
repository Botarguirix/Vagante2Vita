# Iteración 109: diagnóstico del bloqueo de ACT_ONE

La traza nativa de la iteración 107 mostró que el tercer piso se detiene al
procesar `slime_step` para la entidad 18, en el tick 435. No alcanzó a registrar
la salida de esa llamada; por eso aún no se conoce la operación exacta que
consume el tiempo. Se añadieron marcadores internos de entrada y de cada eje
de movimiento para localizarla en la siguiente ejecución.

El barrido compartido de movimiento ahora rechaza velocidades no finitas o
mayores de 16 px por tick antes de convertirlas en un contador de pasos. El
actualizador también restablece esas velocidades en enemigos. El límite está
por encima de las velocidades válidas reconstruidas (máximo observado: 8 px por
tick). Se añadió un caso C para NaN y velocidad desbordada, pero no pudo
ejecutarse en esta sesión.
El conjunto de fuentes del test sí pasó `clang -fsyntax-only` con
`-Wall -Wextra -Werror`; esto comprueba compilación, no la ejecución de la
prueba.

VitaSDK para Windows compiló `_local/build-vita-109/Vagante2Vita-iteration109.vpk`
(835,860 bytes, SFO `APP_VER=01.09`, TitleID `VAGV00006`, SHA-256
`EF5832917471A713D2CFAC6ACE16AD012B804D26FC66D5677E095F4D44F01C66`). El ZIP
contiene los siete archivos esperados y pasa la comprobación de integridad.
El build Windows ahora toma el `APP_VER` de `vita/iteration.cmake`; antes
empaquetaba `00.09` mientras el runtime reportaba `01.09`. El smoke también
compara las versiones de SFO, Vita3K y el log nativo con el formato correcto
para iteraciones 100+. Una prueba de regresión cubre la versión 01.09. La suite
Python pasó 171/171. WSL devolvió `E_ACCESSDENIED`, así que CTest no se
ejecutó. Los proofs v3 y el replay de campaña se regeneraron desde las rutas
exitosas guardadas; los inputs de jugador no cambiaron con los marcadores ni
con el guard de movimiento de enemigos.

La campaña 01.09 se lanzó en Vita3K tras corregir el número de versión, pero
volvió a aparecer un diálogo de error antes de crear una sesión nativa. Su
captura agotó el tiempo, así que el texto del aviso no se pudo leer. Collect
registró `failure_stage=before_native_session`, código de salida no disponible y `vita3k.log` vacío en
`_local/vita3k-vagante-test105-long/runs/iteration109/run-000001/result.json`.
La captura del diálogo agotó el tiempo en dos intentos; se cerró únicamente el
proceso 22016, perteneciente a este test. Por tanto, la versión 01.09 aún no se
ha validado en Vita3K. ACT_ONE sigue incompleto y los pisos 1–2 conservan
geometría provisional, paridad de generación pendiente y transiciones nativas
sin verificar.
