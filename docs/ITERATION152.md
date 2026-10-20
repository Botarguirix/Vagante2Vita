# Iteración 01.52: reintento con copia completa y evidencia de crash

Se volvió a probar el VPK 01.50 en una copia aislada nueva del perfil completo
de Vita3K 0.2.1.4183 (2,162 archivos verificados por SHA-256). El arnés validó
los 142 archivos del paquete ACT_ONE, las imágenes requeridas y el replay de
campaña de 607 pasos / 4,140 ticks / tres pisos. El VPK probado conserva SHA-256
`8249AA136335E3813263E00402EE7288795C43CE888EB6A10B17F48D1B9A773B`.

La ejecución visible con frontend Qt y backend OpenGL terminó con
`0xC0000005` antes de crear un log nativo. No apareció ventana
(`MainWindowHandle=0` durante la observación); stdout, stderr y los logs del runtime para esta
corrida están vacíos. El reporte de la prueba queda en
`_local/vita3k-vagante-fullcopy-run152/runs/iteration150/run-000001/`.
El proceso fue el ejecutable aislado en esa carpeta; el proceso Vita3K de
Touhou detectado después estaba en su propio directorio y no fue modificado.

La inspección del perfil encontró logs en su raíz con fecha del 12 de octubre,
heredados al clonar; no corresponden a esta corrida y no se usan como evidencia.
La prueba confirma que la copia completa y el VPK pueden prepararse, pero sigue
sin localizar la causa del crash de Vita3K ni validar el juego dentro del
emulador. Tampoco prueba el recorrido de campaña. El VPK no cambió y el mundo
ACT_ONE permanece incompleto: siguen pendientes el combate físico fiable del
Dragon, Worm, paridad de generación y transición validada.

Verificación adicional de esta iteración: 204 pruebas Python pasan y los 13
grupos de pruebas C host pasan. No se reconstruyó el VPK porque no cambió el
código del runtime Vita.
