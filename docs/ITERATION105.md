# Iteración 105: salto periódico del Slime y referencias de decompilación

`Slime::Slime @ 0x0078f0c0` inicializa `jumpTimer` en el rango 80–200;
`Slime::step @ 0x0078fab0` lanza un salto con componente horizontal y vertical
aleatorias y gravedad. Se añadió al runtime un ciclo de salto, movimiento por
tiles y hasta dos rebotes amortiguados. Se mantienen las condiciones de la
embestida normal de la iteración 104. El movimiento, la aleatoriedad, el
aterrizaje y los rebotes son aproximaciones: VWE3 no transporta `SlimeType`, y
el runtime no tiene la información necesaria para seleccionar variantes.

`decomp.wiki` se revisó como referencia educativa de conceptos, técnicas,
herramientas y bibliografía de descompilación. No ofrece código de Vagante ni
una herramienta directamente integrable en este port. La diferencia clave
entre matching decompilation y reimplementación funcional quedó documentada
en [REFERENCE_REPOSITORIES.md](REFERENCE_REPOSITORIES.md).

CTest en WSL pasó 12/12. VitaSDK generó
`_local/build-vita-105/Vagante2Vita-iteration105.vpk` (834,227 bytes), TitleID
`VAGV00006`, versión `01.05`, SHA-256
`647BF93A379476C04E3BF47EC18DBD9E33F36A3360E57F4F847D1212A3C4B4CB`.

Los route probes alcanzaron las salidas de los pisos 0/1/2 en 1,741/736/375
ticks. Se recrearon proofs v3 y trazas para el fingerprint del simulador 105;
el paquete privado `_local/delivery-act-one-campaign105` valida 113 archivos.
Esto verifica la simulación host, no la ejecución del VPK en Vita.

No se repitió el smoke de Vita3K: las corridas recientes con el mismo runtime
aislado fallan antes de crear una sesión nativa (`0xC0000005`), incluso con un
VPK de control y con renderer y perfil alternativos. El siguiente intento
requiere obtener un log nativo o corregir el entorno del emulador; no hay
validación de gameplay en Vita3K/hardware. ACT_ONE sigue incompleto.

Una segunda validación con los fuentes actuales separó el temporizador del salto (slime_jump_timer) del temporizador de carga de embestida (slime_lunge_timer); CTest volvió a pasar 12/12. Los proofs y el paquete de entrega deben regenerarse con el fingerprint resultante. Vita3K se abrió con frontend Qt visible, OpenGL y perfil aislado; el proceso permaneció activo sin log ni sesión nativa. Se cerró solo ese proceso y Collect registró efore_native_session sin código de salida (_local/vita3k-vagante-test105/runs/iteration105/run-000002/result.json). La UI se quedó sin ventana visible; no demuestra que el juego haya arrancado.
