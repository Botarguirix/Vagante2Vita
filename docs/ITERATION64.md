# Iteración 064: tabla VLT1 validada antes de empaquetar la campaña

`prepare_act_one_campaign.py` valida ahora el formato completo VLT1 v2 antes
de extender ACT_ONE: tamaño exacto, pools duplicados o fuera de límites,
rangos de filas/aristas, flags, rareza y existencia del pool anidado de cofres
(ID 23). También aplica los límites usados por el runtime nativo. La campaña
incluye en `campaign.json` conteos y SHA-256 de la tabla. La prueba de paquetes
usa una tabla mínima válida, verifica que se conserva byte por byte y rechaza
un archivo VLT1 truncado.

La comprobación independiente del pack privado actual encontró VLT1 v2 con
19 pools directos, 170 filas, 7 pools anidados, 32 conexiones y 3082 bytes;
los hashes de los recursos principales de los pisos 1 y 2 coinciden con el
manifiesto. La campaña se regeneró con el nuevo preparador como
`_local/world-data064/act-one`; su manifiesto valida 78 archivos y conserva
los tres route proofs asociados a los datos por SHA-256.

El VPK `_local/build-act-one-064/Vagante2Vita-iteration064.vpk` se compiló
con VitaSDK. Tiene 825066 bytes y SHA-256
`6B6423EFBDF06D0527B3063CE3DCDEAA8E3ADE7FB39FC75C7BB263DA98B8117E`.
El contenedor abre con las siete entradas esperadas y `git diff --check`
terminó sin errores. La suite Python completa pasa 137 pruebas, incluidas las
de generación con veinte semillas y las nuevas comprobaciones del bundle.

Vita3K sigue sin arrancar desde clones aislados de pruebas y expira sin crear
logs nativos. Se probaron el frontend SDL con instalación por ID y replay de
salida ACT_ONE, y el modo consola OpenGL con carga directa del VPK; ambos
permanecieron 60/30 segundos respectivamente con stdout/stderr y logs vacíos.
El primer intento instaló los siete archivos del VPK en el filesystem aislado,
pero no produjo una sesión de juego. Aún no hay evidencia de ejecución del VPK
en el emulador. Los pisos incluidos son reconstrucciones provisionales;
paridad exacta, entidades completas, combate y la transición de salida aún
requieren validación reproducible para declarar el primer mundo terminado.
